QT       += core gui network quick

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

RC_FILE += resources/icons/main.rc
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    qmess.cpp \
    tools/tools.cpp

HEADERS += \
    qmess.h \
    tools/tools.h

FORMS += \
    qmess.ui

TRANSLATIONS += \
    QMess_ua.ts

RESOURCES += \
    qml.qrc \
    resources/resource.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    CButton.qml \
    CIcon.qml \
    LogInPage.qml \
    PasswordResetPage.qml \
    RegisterScreen.qml \
    UserInfoPage.qml \
    main.qml
