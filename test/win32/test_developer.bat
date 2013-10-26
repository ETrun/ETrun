cd ../../
xcopy /Y build\cgame_mp_x86.dll etrun\
xcopy /Y build\ui_mp_x86.dll etrun\
del /Q etrun.pk3
cd etrun
..\test\win32\zip.exe -r ..\etrun.pk3 *
del /Q *.dll
cd ..\
xcopy /Y etrun.pk3 C:\GameDev\ETtest\etrun\
xcopy /Y etrun.pk3 "C:\Program Files (x86)\ET\etrun\"
del /Q etrun.pk3
xcopy /Y build\*.dll C:\GameDev\ETtest\etrun\
xcopy /Y build\timeruns.mod C:\GameDev\ETtest\etrun\

xcopy /Y etrun\custommapscripts\* "C:\GameDev\ETtest\etrun\custommapscripts\"
xcopy /Y libs\geoip\GeoIP.dat "C:\GameDev\ETtest\etrun\"

echo Press ENTER to start game...
pause
cd ..\
C:\GameDev\ETtest\ETDEDd.exe +set fs_basepath "C:\GameDev\ETtest\" +set fs_game etrun +map test +set developer 1 +set dedicated 1 +set g_useAPI 0 +set g_APImoduleName "timeruns.mod"
