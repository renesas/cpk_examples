set arg1=%1
arm-none-eabi-objdump.exe --dwarf=info %arg1%.elf>%arg1%.txt
..\src\application\rmw\ElfMapConverter.exe %arg1%.txt
copy %arg1%.rmap ..\src\application\rmw\%arg1%_conv.map
