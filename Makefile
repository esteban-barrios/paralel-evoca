semilla = 123
MaxM = 10
MaxEval = 20
NSeeds = 3
MaxTime = 0
Min = 1 #1=minimizar, 0=maximizar
isParallel = 1 # 1=paralelo | 0= secuencial
NumCores = 4
prog = pevoca
archivo_configuracion = config/conf-AK.config
archivo_candidatas = config/candidatas.config
directorio = config/instancias.config
ejecutable = algoritmo_objetivo/AntKnapsack/AK_OSX.sh

$(prog): 
	g++ -Wall pevoca.cpp -o pevoca -pthread

clean:
	rm -f pevoca
	rm -f *.res
	rm -f *~
	rm -f *.o
	rm -f *.conv

exe:$(prog)
	./$(prog) $(ejecutable) $(archivo_configuracion) $(directorio) $(semilla) $(NSeeds) $(MaxM) $(MaxEval) $(MaxTime) $(Min) $(archivo_candidatas) $(isParallel) $(NumCores)