# Implementación Paralela de EVOCA
Compatible con sistemas macOS y distribuciones Linux.

## Pre-requisitos:
Es necesario tener instalado g++ en la distribución.

## Uso:

1- Antes de utilizar Parallel EVOCA, es necesario configurar previamente el algoritmo objetivo a sintonizar. Para ello, se requiere un archivo ejecutable (sh) y su correspondiente compilación (bin) para poder ejecutarlo con EVOCA. También se deben tener las instancias benchmark en las que se desea probar el algoritmo para calcular las aptitudes de sus configuraciones candidatas.

2- Una vez completado el primer paso, se procede a configurar 3 archivos para la configuración de EVOCA: candidatas.config indica si el diseñador presenta algunas configuraciones para incluir en la población inicial, conf-AK es la configuración requerida por el algoritmo para establecer las configuraciones candidatas. La primera línea indica el número de parámetros que se van a sintonizar, luego siguen n líneas para cada parámetro, indicando el nombre y su rango. Finalmente, en la última línea se configura la precisión que tendrá el algoritmo con los valores de los parámetros. Por último, está instancias.config, que nos indica qué instancias entregadas se utilizarán para probar la aptitud del algoritmo.

3- Ahora es necesario completar el makefile, estableciendo las variables correspondientes con los valores deseados para iniciar EVOCA y con los archivos de configuración. Se debe tener en cuenta que se puede elegir entre una ejecución secuencial (0) o paralela (1) cambiando el valor de la variable.

4- Por último, abrir una terminal desde el directorio raíz, ejecutar el comando "make" para compilar PEVOCA, luego ejecutar el comando "make exe" para ejecutarlo en la consola. También se puede utilizar "nohup make exe > output.txt" para registrar las salidas de la ejecución en un archivo y correr el programa en segundo plano.

