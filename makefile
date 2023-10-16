


all:	memo1.exe



memo1.exe : memo1.obj memo1.res
	cl memo1.obj memo1.res kernel32.lib user32.lib gdi32.lib shell32.lib

memo1.res : memo1.rc memo1.h
	rc memo1.rc

memo1.obj : memo1.cpp memo1.h
	cl /c memo1.cpp



