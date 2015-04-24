#include "HifiMenu.h"
#include <QtQml>

// FIXME can this be made a class member?
static const QString MENU_SUFFIX{ "__Menu" };

HIFI_QML_DEF_LAMBDA(HifiMenu, [=](QQmlContext* context, QObject* newItem) {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    QObject * rootMenu = offscreenUi->getRootItem()->findChild<QObject*>("rootMenu");
    Q_ASSERT(rootMenu);
    static_cast<HifiMenu*>(newItem)->setRootMenu(rootMenu);
    context->setContextProperty("rootMenu", rootMenu);
});

HifiMenu::HifiMenu(QQuickItem* parent) : QQuickItem(parent), _triggerMapper(this), _toggleMapper(this) {
    this->setEnabled(false);
    connect(&_triggerMapper, SIGNAL(mapped(QString)), this, SLOT(onTriggeredByName(const QString &)));
    connect(&_toggleMapper, SIGNAL(mapped(QString)), this, SLOT(onToggledByName(const QString &)));
}

void HifiMenu::onTriggeredByName(const QString & name) {
    qDebug() << name << " triggered";
    if (_triggerActions.count(name)) {
        _triggerActions[name]();
    }
}

void HifiMenu::onToggledByName(const QString & name) {
    qDebug() << name << " toggled";
    if (_toggleActions.count(name)) {
        QObject* menu = findMenuObject(name);
        bool checked = menu->property("checked").toBool();
        _toggleActions[name](checked);
    }
}

void HifiMenu::setToggleAction(const QString & name, std::function<void(bool)> f) {
    _toggleActions[name] = f;
}

void HifiMenu::setTriggerAction(const QString & name, std::function<void()> f) {
    _triggerActions[name] = f;
}

QObject* addMenu(QObject* parent, const QString & text) {
    // FIXME add more checking here to ensure no name conflicts
    QVariant returnedValue;
    QMetaObject::invokeMethod(parent, "addMenu",
        Q_RETURN_ARG(QVariant, returnedValue),
        Q_ARG(QVariant, text));
    QObject* result = returnedValue.value<QObject*>();
    if (result) {
        result->setObjectName(text + MENU_SUFFIX);
    }
    return result;
}

class QQuickMenuItem;
QObject* addItem(QObject* parent, const QString& text) {
    // FIXME add more checking here to ensure no name conflicts
    QQuickMenuItem* returnedValue{ nullptr };
    bool invokeResult = QMetaObject::invokeMethod(parent, "addItem",
        Q_RETURN_ARG(QQuickMenuItem*, returnedValue),
        Q_ARG(QString, text));
    Q_ASSERT(invokeResult);
    QObject* result = reinterpret_cast<QObject*>(returnedValue);
    return result;
}

const QObject* HifiMenu::findMenuObject(const QString & menuOption) const {
    if (menuOption.isEmpty()) {
        return _rootMenu;
    }
    const QObject* result = _rootMenu->findChild<QObject*>(menuOption + MENU_SUFFIX);
    return result;
}

QObject* HifiMenu::findMenuObject(const QString & menuOption) {
    if (menuOption.isEmpty()) {
        return _rootMenu;
    }
    QObject* result = _rootMenu->findChild<QObject*>(menuOption + MENU_SUFFIX);
    return result;
}

void HifiMenu::addMenu(const QString & parentMenu, const QString & menuOption) {
    QObject* parent = findMenuObject(parentMenu);
    QObject* result = ::addMenu(parent, menuOption);
    Q_ASSERT(result);
    result->setObjectName(menuOption + MENU_SUFFIX);
    Q_ASSERT(findMenuObject(menuOption));
}

void HifiMenu::removeMenu(const QString& menuName) {
    QObject* menu = findMenuObject(menuName);
    Q_ASSERT(menu);
    Q_ASSERT(menu != _rootMenu);
    QMetaObject::invokeMethod(menu->parent(), "removeItem",
        Q_ARG(QVariant, QVariant::fromValue(menu)));
}

bool HifiMenu::menuExists(const QString& menuName) const {
    return findMenuObject(menuName);
}

void HifiMenu::addSeparator(const QString& parentMenu, const QString& separatorName) {
    // FIXME 'add sep'
//    addMenu(parentMenu, separatorName);
//    setEnabled()
}

void HifiMenu::removeSeparator(const QString& parentMenu, const QString& separatorName) {
}

void HifiMenu::addMenuItem(const QString & parentMenu, const QString & menuOption) {
    QObject* parent = findMenuObject(parentMenu);
    Q_ASSERT(parent);
    QObject* result = ::addItem(parent, menuOption);
    Q_ASSERT(result);
    result->setObjectName(menuOption + MENU_SUFFIX);
    Q_ASSERT(findMenuObject(menuOption));

    _triggerMapper.setMapping(result, menuOption);
    connect(result, SIGNAL(triggered()), &_triggerMapper, SLOT(map()));

    _toggleMapper.setMapping(result, menuOption);
    connect(result, SIGNAL(toggled(bool)), &_toggleMapper, SLOT(map()));
}

void HifiMenu::addMenuItem(const QString & parentMenu, const QString & menuOption, std::function<void()> f) {
    setTriggerAction(menuOption, f);
    addMenuItem(parentMenu, menuOption);
}

void HifiMenu::removeMenuItem(const QString& menuOption) {
    removeMenu(menuOption);
}

bool HifiMenu::menuItemExists(const QString& menuName, const QString& menuitem) const {
    return findMenuObject(menuName);
}

void HifiMenu::triggerMenuItem(const QString& menuOption) {
    QObject* menuItem = findMenuObject(menuOption);
    Q_ASSERT(menuItem);
    Q_ASSERT(menuItem != _rootMenu);
    QMetaObject::invokeMethod(menuItem, "trigger");
}

QHash<QString, QString> warned;
void warn(const QString & menuOption) {
    if (!warned.contains(menuOption)) {
        warned[menuOption] = menuOption;
        qWarning() << "No menu item: " << menuOption;
    }
}

bool HifiMenu::isChecked(const QString& menuOption) const {
    const QObject* menuItem = findMenuObject(menuOption);
    if (!menuItem) {
        warn(menuOption); 
        return false;
    }
    return menuItem->property("checked").toBool();
}

void HifiMenu::setChecked(const QString& menuOption, bool isChecked) {
    QObject* menuItem = findMenuObject(menuOption);
    if (!menuItem) {
        warn(menuOption);
        return;
    }
    menuItem->setProperty("checked", QVariant::fromValue(isChecked));
    Q_ASSERT(menuItem->property("checked").toBool() == isChecked);
}

void HifiMenu::setCheckable(const QString& menuOption, bool checkable) {
    QObject* menuItem = findMenuObject(menuOption);
    if (!menuItem) {
        warn(menuOption);
        return;
    }
    
    menuItem->setProperty("checkable", QVariant::fromValue(checkable));
    Q_ASSERT(menuItem->property("checkable").toBool() == checkable);
}

void HifiMenu::setText(const QString& menuOption, const QString & text) {
    QObject* menuItem = findMenuObject(menuOption);
    if (!menuItem) {
        warn(menuOption);
        return;
    }
    menuItem->setProperty("text", QVariant::fromValue(text));
}

void HifiMenu::setRootMenu(QObject* rootMenu) {
    _rootMenu = rootMenu;
}

void HifiMenu::enableMenuItem(const QString & menuOption, bool enabled) {
    QObject* menuItem = findMenuObject(menuOption);
    if (!menuItem) {
        warn(menuOption);
        return;
    }
    menuItem->setProperty("enabled", QVariant::fromValue(enabled));
}

void HifiMenu::addCheckableMenuItem(const QString& parentMenu, const QString& menuOption, bool checked) {
    addMenuItem(parentMenu, menuOption);
    setCheckable(menuOption);
    if (checked) {
        setChecked(menuOption, checked);
    }
}

void HifiMenu::addCheckableMenuItem(const QString& parentMenu, const QString& menuOption, bool checked, std::function<void(bool)> f) {
    setToggleAction(menuOption, f);
    addCheckableMenuItem(parentMenu, menuOption, checked);
}
