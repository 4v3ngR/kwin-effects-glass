#!/bin/sh
$EXTRACTRC `find . -name \*.rc -o -name \*.ui -o -name \*.kcfg` >> rc.cpp
$XGETTEXT `find . -name \*.cc -o -name \*.cpp -o -name \*.h -name \*.qml` -o $podir/kwin_effects_glass.pot
rm -f rc.cpp
