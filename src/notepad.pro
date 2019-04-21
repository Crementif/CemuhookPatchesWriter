TEMPLATE = app
TARGET = notepad

qtHaveModule(printsupport): QT += printsupport
requires(qtConfig(fontdialog))

SOURCES += \
    highlighter.cpp \
    main.cpp\
    notepad.cpp

HEADERS += notepad.h \
    highlighter.h

FORMS += notepad.ui

RESOURCES += \
    notepad.qrc

CONFIG+= static

# install
target.path = /
INSTALLS += target

