# Parameters
semilla = 123 # semilla para números aleatorios
MaxM = 10 # núm max de la población
MaxEval = 20 # núm max de evaluaciones
NSeeds = 3 # cantidad de semillas/instancias requeridas para calcular aptitudes
MaxTime = 0 # núm max de tiempo (0 implica considerar solo max eval)
Min = 1 # 1=minimizar | 0=maximizar
isParallel = 1 # 1=paralelo | 0= secuencial
NumCores = 4 # núm de nucleos a utilizar (considerar max el número de nucleos del procesador para evitar problemas)

# Config
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