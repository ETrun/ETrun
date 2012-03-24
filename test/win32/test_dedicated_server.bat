cd ../../
xcopy /Y _build\cgame_mp_x86.dll etrun\
xcopy /Y _build\ui_mp_x86.dll etrun\
del /Q etrun.pk3
cd etrun
..\test\win32\zip.exe -r ..\etrun.pk3 *
del /Q *.dll
cd ..\
xcopy /Y etrun.pk3 C:\Users\Nicolas\Desktop\etrun\
xcopy /Y etrun.pk3 "C:\Program Files (x86)\ET\etrun\"
del /Q etrun.pk3
xcopy /Y _build\qagame_mp_x86.dll C:\Users\Nicolas\Desktop\etrun\
echo Press ENTER to start game...
pause
cd ..\
C:\Users\Nicolas\Desktop\ETDED.exe +set fs_basepath "C:\Users\Nicolas\Desktop\" +set fs_game etrun +map killua-pornstar
