compile-and-run:
	g++ ./main.cpp -I"./include" -L"./lib" -lsfml-graphics-s -lsfml-window-s -lsfml-system-s -lopengl32  -lwinmm  -lgdi32 -DSFML_STATIC 
	.\a.exe
compile:
	g++ ./main.cpp -I"./include" -L"./lib" -lsfml-graphics-s -lsfml-window-s -lsfml-system-s -lopengl32  -lwinmm  -lgdi32 -DSFML_STATIC
