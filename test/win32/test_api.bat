cd ../../
xcopy /Y build\cgame_mp_x86.dll etrun\
xcopy /Y build\ui_mp_x86.dll etrun\
del /Q etrun.pk3
cd etrun
..\test\win32\zip.exe -r ..\etrun.pk3 *
del /Q *.dll
cd ..\
xcopy /Y etrun.pk3 C:\Users\Nicolas\Desktop\etrun\
xcopy /Y etrun.pk3 "C:\Program Files (x86)\ET\etrun\"
del /Q etrun.pk3
xcopy /Y build\*.dll C:\Users\Nicolas\Desktop\etrun\

xcopy /Y etrun\custommapscripts\* "C:\Users\Nicolas\Desktop\etrun\custommapscripts\"
xcopy /Y libs\geoip\GeoIP.dat "C:\Users\Nicolas\Desktop\etrun\"

echo Press ENTER to start game...
pause
cd ..\
C:\Users\Nicolas\Desktop\ETDED.exe +set fs_basepath "C:\Users\Nicolas\Desktop\" +set fs_game etrun +map killua-pornstar +set dedicated 1 +set g_useAPI 1 +set g_APImodulePath "C:\Users\Nicolas\Desktop\etrun\APImodule.dll"
