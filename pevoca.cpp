#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <vector>
#include <fstream>
#include <list>
#include <string.h>
#include <algorithm>
//#include <semaphore.h>
#include <dispatch/dispatch.h>
#include <limits.h>
#include <queue>

using namespace std;

// Structs para EVOCA

struct configuracion {
  char nombre[30];
  int limite_minimo, limite_maximo;
  int decimales;
  int t_dominio;
};
struct si{
  int seed; //id
  string instance; //nombre
};
class calibracion{
  public:
  float aptitud_promedio; // Esto considera solo una instancia a sintonizar
  vector<int> parametro;
  calibracion();
  //   calibracion(const calibracion &);
  ~calibracion(){};
  calibracion& operator=(const calibracion &rhs);
  int operator==(const calibracion &rhs) const;
  bool operator<(const calibracion &rhs) const;
  friend ostream& operator<<(ostream &, const calibracion &);
};
class conjunto{ 
  public: 
  int id;
  vector <calibracion> cjto;
  void vaciar(void);
  void ordenar(void);
  friend ostream& operator<<(ostream &, const conjunto &);
};
class tpoblacion{
  public:
  int id;
  vector <conjunto> cjt;
  void vaciar(void);
  friend ostream& operator<<(ostream &, const tpoblacion &);
};

// type of chrono lib
typedef std::chrono::time_point<std::chrono::high_resolution_clock>  TimePoint;
typedef std::chrono::microseconds Mcs;
typedef std::chrono::duration<float> Fsec;

// Parametros globales EVOCA
int CantEvalsDiscardedMut = 0;
int CantEvalsCrossBad = 0;
int debug = 1;
vector <si> lista_semillas_instancias;
int I, E=0, T=0;
tpoblacion Poblacion, Poblacion_temporal;
struct configuracion *configuracion_parametros;
int cantidad_parametros = 0;
int max_decimales;
int **valores_parametros;
conjunto poblacion, poblacion_intermedia;
int CIT;
TimePoint global_start = chrono::high_resolution_clock::now();
TimePoint global_end = chrono::high_resolution_clock::now();

// Parametros entregados como argumento
char* algoritmoObjetivo;      // ejecutable (sh) del algoritmo objetivo
char* archivo_configuracion;  // dominios iniciales de parametros y precision
char* archivo_instancias;  // path del directorio donde se encuentran las instancias
int SEED;                     // semilla para inicialiar los números aleatorios
int NumSeeds;                 // cantidad de semillas/instancias a probar (K semilla aleatoria)
int MaxM;                     // tamaño max de la poblacion
int MAX_EVAL;                 // max cantidad de evaluaciones
int MAX_TIME;                 // max tiempo de ejecucion
int minimizar;                // 1=minimizar, 0=maximizar
char* archivo_candidatas;     // archivo de soluciones candidatas inciales 
int isParallel; // 0: secuencial y 1: paralelo

// Funciones para Structs

calibracion::calibracion()   // Constructor
{
  aptitud_promedio = 0.00;
}

bool calibracion::operator<(const calibracion &a) const
{   
  if(a.aptitud_promedio > aptitud_promedio) return 1;
  return 0;
}

calibracion& calibracion::operator=(const calibracion &a)
{
  this->aptitud_promedio = a.aptitud_promedio;
  this->parametro = a.parametro;
  return *this;
}

int calibracion::operator==(const calibracion &a) const
{
  if( this->aptitud_promedio != a.aptitud_promedio) return 0;
  return 1;
}

ostream& operator<<(ostream &output, const calibracion &c)
{
  output << "[" << &c << "] ( ";
  for(int i=0; i<cantidad_parametros; i++)
    output << c.parametro[i] << " ";
  output << ") ["<< c.aptitud_promedio<<"]"<<endl;
  return output;
}

void conjunto::vaciar(void)
{
  int size = cjto.size();
  for(int i=0; i<size; i++){
    cjto[i].parametro.clear();
  }
  cjto.clear();
}

ostream& operator<<(ostream &output, const conjunto &co)
{
  output <<endl<< "--------------Conjunto "<<co.id+1<<" ("<<co.cjto.size()<<")-----------------------"<<endl;
  if(co.cjto.size()>0)
  {
    for (vector<calibracion>::const_iterator ca = co.cjto.begin(); ca!=co.cjto.end(); ++ca)
    {
      output<<*ca;   
    }
  }
  return output;
}

void tpoblacion::vaciar(void)
{
  int size = cjt.size();
  for(int i=0; i<size; i++){
    cjt[i].cjto.clear();
  }
  cjt.clear();
}

ostream& operator<<(ostream &output, const tpoblacion &po)
{
  output << "--------------Poblacion "<<po.id+1<<" ("<<po.cjt.size()<<")----------------------"<<endl;
  if(po.cjt.size()>0)
  {
    for (vector<conjunto>::const_iterator co = po.cjt.begin(); co!=po.cjt.end(); ++co)
    {
      output<<*co;   
    }
  }
  return output;
}

// Funciones globales
int int_rand (int a, int b)
{ 
  //drand48() devuelve valores en el intervalo [0.0, 1.0)
  int retorno = 0;
  if (a < b)
  {
    retorno = (int) ((b - a) * drand48 ());
    retorno = retorno + a;
  }
  else
  {
    retorno = (int) ((a - b) * drand48 ());
    retorno = retorno + b;
  }
  return retorno;
}


void salir(void)
{ 
  FILE *archivo;
  // assigning value to string s
  string s = "EVOCA.params";
  const int length = s.length();
  // declaring character array (+1 for null terminator)
  char* archivo_parametros = new char[length + 1];
  // copying the contents of the string to char array
  strcpy(archivo_parametros, s.c_str());

  if(debug) printf("Archivo_parametros: %s\n", archivo_parametros);
  archivo = fopen (archivo_parametros, "w");
  fprintf(archivo,"%s ", archivo_instancias); 
   
  for(int p=0; p<cantidad_parametros; p++){
    if(configuracion_parametros[p].decimales > 1)
      fprintf(archivo," %.5f ", poblacion.cjto[0].parametro[p]/(float)configuracion_parametros[p].decimales);
    else
      fprintf(archivo, "%d ", poblacion.cjto[0].parametro[p]);
  }
  fprintf(archivo,"\n"); 
  fclose(archivo);


  cout << "Evaluaciones Perdidas por Mutaciones -->" << CantEvalsDiscardedMut << endl;
  int tot= (int)CantEvalsDiscardedMut/NumSeeds;
  cout << "Configuraciones descartadas por Mutacion -->" << tot << endl;
  tot = (int)CantEvalsCrossBad/NumSeeds;
  cout << "Evaluaciones Perdidas por Cruzamiento -->" << CantEvalsCrossBad << endl;
  cout << "Configuraciones descartadas por Cruzamiento -->" << tot << endl;

  global_end = chrono::high_resolution_clock::now();
  Mcs dur= chrono::duration_cast<chrono::microseconds>(global_end - global_start); // calcular la duración en microsegundos
  double dur_sec = static_cast<double>(dur.count()) / 1000000.0; // calcular la duración en segundos
  cout << "Tiempo de Total de Ejecucion: " << dur_sec << " segundos" << endl;
  exit(0);
  return;
}


/*-----------------------------------------------------------------*/
//Funciones evoca paralelo


// Parametro para indicar el número de hebras que se pueden ejecutar en paralelo
int numCore = 4; 
// variables para crear hebras
int numWorkers = numCore -1 ;         

// Struct para pasar valores diferentes a las hebras
struct parametrosHebra {
    calibracion *cal_temp;
    const char* ins;
    int sem;
    int i;
};
  
// Definimos un semáforo global
dispatch_semaphore_t semaforo;

// Def para work-queue
dispatch_semaphore_t queue_sem;
dispatch_semaphore_t task_sem;
bool all_tasks_processed = false;
queue<parametrosHebra> work_queue;

// calcular aptitud de semilla con work-queue
void* hebra_calcular_aptitud_semilla_instancia(void* arg) {
  while (true) {
    // Esperar a que haya tareas en la cola
    dispatch_semaphore_wait(queue_sem, DISPATCH_TIME_FOREVER);

    // Bloquear el semáforo para acceso exclusivo a la cola de trabajo
    dispatch_semaphore_wait(task_sem, DISPATCH_TIME_FOREVER);

    // Comprobar si todas las tareas han sido procesadas
    if (all_tasks_processed && work_queue.empty()) {
        // Desbloquear el semáforo y salir del bucle
        dispatch_semaphore_signal(task_sem);
        break;
    }

    // Obtener la tarea de la cola
    parametrosHebra task = work_queue.front();
    work_queue.pop();

    // Desbloquear el semáforo
    dispatch_semaphore_signal(task_sem);

    // Realizar la tarea
    TimePoint start = chrono::high_resolution_clock::now();

    calibracion *cal_temp = task.cal_temp;
    const char *instancia = task.ins;
    int semilla = task.sem;
    int s = task.i;

    // nombre archivo
    char archivo_resultados[1000]; 
    strcpy (archivo_resultados, "resultados_");
    snprintf(archivo_resultados + strlen(archivo_resultados), sizeof(archivo_resultados) - strlen(archivo_resultados), "%d", s);
    strcat(archivo_resultados, ".res"); // Agregar ".res" al final de cada cadena
  
    ifstream resultados;

    char comando[1024];
    char comando2[1024];
    float aptitud;
    
    // sprintf(comando, "bash %s %s %s %d", algoritmoObjetivo, instancia, archivo_resultados, semilla);
    // for(int i=0; i<cantidad_parametros; i++)
    // {
    //   if(configuracion_parametros[i].decimales>1)
    //     sprintf(comando2," -%s %.4f", configuracion_parametros[i].nombre,(cal_temp->parametro[i]/((float)configuracion_parametros[i].decimales)));
    //   else
    //     sprintf(comando2," -%s %d", configuracion_parametros[i].nombre, (int)(cal_temp->parametro[i]));
    //   strcat(comando, comando2);
    // }
    // printf("%s\n", comando);

    snprintf(comando, sizeof(comando), "bash %s %s %s %d", algoritmoObjetivo, instancia, archivo_resultados, semilla);
    
    for (int i = 0; i < cantidad_parametros; i++) {
        if (configuracion_parametros[i].decimales > 1)
            snprintf(comando2, sizeof(comando2), " -%s %.4f", configuracion_parametros[i].nombre, (cal_temp->parametro[i] / ((float)configuracion_parametros[i].decimales)));
        else
            snprintf(comando2, sizeof(comando2), " -%s %d", configuracion_parametros[i].nombre, (int)(cal_temp->parametro[i]));
        
        strncat(comando, comando2, sizeof(comando) - strlen(comando) - 1);
    }
    
    printf("%s\n", comando);
    system(comando);

    system(comando);
    resultados.open(archivo_resultados);
    resultados>>aptitud;
    resultados.close();

    if(MAX_TIME!=0) //Cuando MAX_TIME==0, no se considera tiempo limite
      T+=aptitud;
    printf("Aptitud de (%d): %2f \n", s, aptitud);

    // Seccion critica
    dispatch_semaphore_wait(semaforo, DISPATCH_TIME_FOREVER); // bloqueamos
    cal_temp->aptitud_promedio = (cal_temp-> aptitud_promedio + aptitud);
    dispatch_semaphore_signal(semaforo);  // liberamos

    //cal_temp->c_instancias_semillas++;
    printf("Aptitud Total: %2f (%d)\n", (cal_temp->aptitud_promedio), (s));

    E++;
    cout<<"-->Van "<< E <<" evaluaciones ("<< T <<" segundos)"<<endl;

    TimePoint end1 = chrono::high_resolution_clock::now(); // tomar el tiempo de finalización
    Mcs dur1 = chrono::duration_cast<chrono::microseconds>(end1 - start); // calcular la duración en microsegundos
    double dur_sec1 = static_cast<double>(dur1.count()) / 1000000.0; // calcular la duración en segundos
    cout << "Tiempo de ejecución individual en par: " << dur_sec1 << " segundos" << "(" << s << ")"<<endl;

    if((MAX_EVAL!=0) && (E>MAX_EVAL)) // Cuando MAX_EVAL==0, no se considera numero de evaluaciones limite
    {
      cout<<"MAX_EVAL!: "<<E<<endl;
      salir();
    }

    if((MAX_TIME!=0) && (T>MAX_TIME)) //Cuando MAX_TIME==0, no se considera tiempo limite
    {
      cout<<"MAX_TIME!: "<<T<<endl;
      salir();
    }

  }

  pthread_exit(NULL);
  
}
// Funcion paralela para calcular aptitudes con work-queue
void calcular_aptitud_calibracion_paralelo(calibracion *cal_temp)
{
  pthread_t hebras[numWorkers];  
  all_tasks_processed = false;
  // Tomar el tiempo de inicio
  TimePoint start = chrono::high_resolution_clock::now();
  cout << "-----Se inicia iteracion de semillas------" << endl; 

  // Inicializar los semáforos
  queue_sem = dispatch_semaphore_create(0); // init with value of 0
  task_sem = dispatch_semaphore_create(1); // init with value of 1
  semaforo = dispatch_semaphore_create(1); // init with value of 1

  // Crear los hilos
  for (int i = 0; i < numWorkers; i++) {
      pthread_create(&hebras[i], NULL, hebra_calcular_aptitud_semilla_instancia, NULL);
  }


  if (NumSeeds > numWorkers){
    for(int i=0; i< NumSeeds; i++) {
      // Obtener semilla y instancia
      int is=int_rand(0,lista_semillas_instancias.size());
      int sem=lista_semillas_instancias[is].seed;
      const char *ins = lista_semillas_instancias[is].instance.c_str();

      // Inicializar parametros(tarea) de hebra
      parametrosHebra argumento;
      argumento.cal_temp = cal_temp;
      argumento.sem = sem;
      argumento.ins = ins;
      argumento.i = i;

      // Bloquear el semáforo para acceso exclusivo a la cola de trabajo
      dispatch_semaphore_wait(task_sem, DISPATCH_TIME_FOREVER);

      work_queue.push(argumento);

      // Desbloquear el semáforo
      dispatch_semaphore_signal(task_sem);

      // Desbloquear el semáforo para indicar la disponibilidad de tareas
      dispatch_semaphore_signal(queue_sem);
    }

    // Indicar que todas las tareas han sido procesadas
    all_tasks_processed = true;

    // Desbloquear el semáforo para que los hilos verifiquen si todas las tareas han sido procesadas
    for (int i = 0; i < numWorkers; i++) {
        dispatch_semaphore_signal(queue_sem);
    }

    // Esperar a que las hebras terminen
    for (int i = 0; i < numWorkers; i++) {
        pthread_join(hebras[i], NULL);
    }

    cal_temp->aptitud_promedio = cal_temp->aptitud_promedio/NumSeeds;
    // Destruir los semáforos
    dispatch_release(queue_sem);
    dispatch_release(task_sem);
    dispatch_release(semaforo);
    cout << "-----Todas las tareas han sido procesadas.-----" << endl;


    TimePoint end = chrono::high_resolution_clock::now(); // tomar el tiempo de finalización
    Mcs dur = chrono::duration_cast<chrono::microseconds>(end - start); // calcular la duración en microsegundos
    double dur_sec = static_cast<double>(dur.count()) / 1000000.0; // calcular la duración en segundos

    cout << "Tiempo de ejecución para calcular aptitud en paralelo: " << dur_sec << " segundos" << endl;
    cout << "Aptitud final: " << cal_temp->aptitud_promedio << endl;
  }
  else{
    for (int j = 0; j < NumSeeds; j++) {
      // Obtener semilla y instancia
      int is=int_rand(0,lista_semillas_instancias.size());
      int sem=lista_semillas_instancias[is].seed;
      const char *ins = lista_semillas_instancias[is].instance.c_str();

      // Inicializar parametros(tarea) de hebra
      parametrosHebra argumento;
      argumento.cal_temp = cal_temp;
      argumento.sem = sem;
      argumento.ins = ins;
      argumento.i = j;

      // Bloquear el semáforo para acceso exclusivo a la cola de trabajo
      dispatch_semaphore_wait(task_sem, DISPATCH_TIME_FOREVER);

      work_queue.push(argumento);

      // Desbloquear el semáforo
      dispatch_semaphore_signal(task_sem);

      // Desbloquear el semáforo para indicar la disponibilidad de tareas
      dispatch_semaphore_signal(queue_sem);
    }
    // Indicar que todas las tareas han sido procesadas
    all_tasks_processed = true;

    // Aumentar el semáforo para que los hilos verifiquen si todas las tareas han sido procesadas
    for (int i = 0; i < numWorkers; i++) {
        dispatch_semaphore_signal(queue_sem);
    }

    // Esperar a que los hilos terminen
    for (int i = 0; i < numWorkers; i++) {
        pthread_join(hebras[i], NULL);
    }
    cal_temp->aptitud_promedio = cal_temp->aptitud_promedio/NumSeeds;
    // Destruir los semáforos
    dispatch_release(queue_sem);
    dispatch_release(task_sem);
    dispatch_release(semaforo);
    cout << "-----Todas las tareas han sido procesadas.-----" << endl;

    TimePoint end = chrono::high_resolution_clock::now(); // tomar el tiempo de finalización
    Mcs dur = chrono::duration_cast<chrono::microseconds>(end - start); // calcular la duración en microsegundos
    double dur_sec = static_cast<double>(dur.count()) / 1000000.0; // calcular la duración en segundos

    cout << "Tiempo de ejecución para calcular aptitud en paralelo: " << dur_sec << " segundos" << endl;
    cout << "Aptitud final: " << cal_temp->aptitud_promedio << endl;
  }
}


/*-----------------------------------------------------------------*/



// Funciones EVOCA secuencial

void agregar_semilla_instancia(vector <si> *lista)
{
  si si_temp;
  ifstream infile;
  infile.open(archivo_instancias); // open file 
  int sel=int_rand(0, CIT);
  int c=0;
  if(infile){
    string s="";
    while(infile){
      getline(infile,s);
      if(sel==c)
      {
        si_temp.instance=s;
        si_temp.seed=int_rand(0,10000);
        lista_semillas_instancias.push_back(si_temp);
        infile.close(); // file close
        return;
      }
      c++;
    }
  }
  else{
    cout<<"Archivo de instancias no encontrado!";
    infile.close();
  }
  return;
}

void mostrar_lista_semillas_instancias(void){
  for (vector<si>::const_iterator l= lista_semillas_instancias.begin(); l!= lista_semillas_instancias.end(); ++l){
    int sem=l->seed;
    const char *ins = l->instance.c_str();
    cout<<"S:"<<sem<<" I:"<<ins<<endl;
  }
  //getchar();
  return; 
}

void leer_archivo_configuracion(void){
  FILE *archivo;
  archivo=fopen(archivo_configuracion, "r");
  fscanf(archivo, "%d", &cantidad_parametros);
  cout<<cantidad_parametros<<endl;

  if(cantidad_parametros < 0){
    printf("Cantidad de parametros invalida");
    fclose(archivo);
    exit(1);
  }
  
  configuracion_parametros = new configuracion[cantidad_parametros];
  
  for(int i=0; i< cantidad_parametros; i++){
    fscanf(archivo, "%s", configuracion_parametros[i].nombre);
    printf("nombre leido: %s\n", configuracion_parametros[i].nombre);
    fscanf(archivo, "%d", &configuracion_parametros[i].limite_minimo);
    printf("lim_min: %d\n", configuracion_parametros[i].limite_minimo);
    fscanf(archivo, "%d", &configuracion_parametros[i].limite_maximo);
    printf("lim_max: %d\n", configuracion_parametros[i].limite_maximo);
    fscanf(archivo, "%d", &configuracion_parametros[i].decimales);
    printf("decimales: %d\n", configuracion_parametros[i].decimales);
  }
  
  fscanf(archivo, "%d", &max_decimales);
  fclose(archivo);
  if(debug) printf("leer_archivo_configuracion done!\n");
  return;
}


void calcular_aptitud_semilla_instancia(calibracion *cal_temp, const char* instancia, int semilla, int s)
{

  ifstream resultados;
  
  char archivo_resultados[1000];
  char archivo_sh[1000];
  strcpy (archivo_resultados, "resultados.res");
  strcpy (archivo_sh, "ejecutar_");
  strcat (archivo_sh, archivo_instancias);
  strcat (archivo_sh, ".sh");

  char comando[1024];
  char comando2[1024];
  float aptitud;

  //for(int i=0; i< cantidad_parametros; i++)
  //  cout<<cal_temp->parametro[i]<<" ";
  //cout<<endl;

  fflush(stdout);
  //sprintf(comando, "rm -rf %s", archivo_resultados);
  snprintf(comando, sizeof(comando), "rm -rf %s", archivo_resultados);
  system(comando);

  // sprintf(comando, "bash %s %s %s %d", algoritmoObjetivo, instancia, archivo_resultados, semilla);
  // for(int i=0; i<cantidad_parametros; i++)
  // {
  //   if(configuracion_parametros[i].decimales>1)
  //     sprintf(comando2," -%s %.4f", configuracion_parametros[i].nombre,(cal_temp->parametro[i]/((float)configuracion_parametros[i].decimales)));
  //   else
  //     sprintf(comando2," -%s %d", configuracion_parametros[i].nombre, (int)(cal_temp->parametro[i]));
  //   strcat(comando, comando2);
  // }

  // printf("%s\n", comando);
  // system(comando);

  snprintf(comando, sizeof(comando), "bash %s %s %s %d", algoritmoObjetivo, instancia, archivo_resultados, semilla);
  
  for (int i = 0; i < cantidad_parametros; i++) {
      if (configuracion_parametros[i].decimales > 1)
          snprintf(comando2, sizeof(comando2), " -%s %.4f", configuracion_parametros[i].nombre, (cal_temp->parametro[i] / ((float)configuracion_parametros[i].decimales)));
      else
          snprintf(comando2, sizeof(comando2), " -%s %d", configuracion_parametros[i].nombre, (int)(cal_temp->parametro[i]));
      
      strncat(comando, comando2, sizeof(comando) - strlen(comando) - 1);
  }
  
  printf("%s\n", comando);
  system(comando);
  
  resultados.open(archivo_resultados);
  resultados>>aptitud;
  resultados.close();
    
  if(MAX_TIME!=0) //Cuando MAX_TIME==0, no se considera tiempo limite
    T+=aptitud;

  cal_temp->aptitud_promedio = (cal_temp->aptitud_promedio*(s) + aptitud)/((s)+1);
  //cal_temp->c_instancias_semillas++;
  printf("Aptitud Total: %2f (%d)\n", (cal_temp->aptitud_promedio), (s));
  
  E++;
  cout<<"-->Van "<< E <<" evaluaciones ("<< T <<" segundos)"<<endl;
  if((MAX_EVAL!=0) && (E>MAX_EVAL)) // Cuando MAX_EVAL==0, no se considera numero de evaluaciones limite
  {
    cout<<"MAX_EVAL!: "<<E<<endl;
    salir();
  }

  if((MAX_TIME!=0) && (T>MAX_TIME)) //Cuando MAX_TIME==0, no se considera tiempo limite
  {
    cout<<"MAX_TIME!: "<<T<<endl;
    salir();
  }
  return;
}

void calcular_aptitud_calibracion(calibracion *cal_temp)
{
  if(isParallel){
    calcular_aptitud_calibracion_paralelo(cal_temp);
    return;
  }
  else{
    // Tomar el tiempo de inicio
    TimePoint start = chrono::high_resolution_clock::now();
    cout << "Se inicia iteracion de semillas" << endl; 
    for(int i=0; i<NumSeeds; i++)
    {
      int is=int_rand(0,lista_semillas_instancias.size());
      //cout<<"is: "<<is<<endl;
      int sem=lista_semillas_instancias[is].seed;
      const char *ins = lista_semillas_instancias[is].instance.c_str();
      TimePoint start1 = chrono::high_resolution_clock::now();
      calcular_aptitud_semilla_instancia(cal_temp, ins, sem, i);
      TimePoint end1 = chrono::high_resolution_clock::now(); // tomar el tiempo de finalización
      Mcs dur1 = chrono::duration_cast<chrono::microseconds>(end1 - start1); // calcular la duración en microsegundos
      double dur_sec1 = static_cast<double>(dur1.count()) / 1000000.0; // calcular la duración en segundos
      cout << "Tiempo de ejecución individual en sec: " << dur_sec1 << " segundos" << endl;
    }
    TimePoint end = chrono::high_resolution_clock::now(); // tomar el tiempo de finalización
    Mcs dur = chrono::duration_cast<chrono::microseconds>(end - start); // calcular la duración en microsegundos
    double dur_sec = static_cast<double>(dur.count()) / 1000000.0; // calcular la duración en segundos

    cout << "Tiempo de ejecución para calcular aptitud en sec: " << dur_sec << " segundos" << endl;
    cout << "Aptitud final: " << cal_temp->aptitud_promedio << endl;
    return;
  }
}

void calcular_aptitud_conjunto_inicial(conjunto *co){
  for (vector < calibracion >::iterator ca=co->cjto.begin (); ca != co->cjto.end (); ++ca){
    calcular_aptitud_calibracion(&*ca);
  }
  return;
}

int seleccionar_valor_parametro(conjunto * co, int p, int N){
  int pos = int_rand(0, N); //entre 0 y N-1
  int valor=co->cjto[pos].parametro[p];
  return valor;
}

int seleccionar_valor_parametro_por_ruleta(conjunto * co, float * aptitud_temporal, float total, int p)
{
  float rand=drand48();
  float aptitud_acumulada=0.00;
  int M=co->cjto.size();
    
  for (int i=0; i<M; i++)
  {
   aptitud_acumulada += (aptitud_temporal[i]/total);
   if(rand < aptitud_acumulada)
   {
     int sel=co->cjto[i].parametro[p];
     return sel;
   }
  }
  return (co->cjto.end())->parametro[p];
}

float calcular_aptitud_temporal(conjunto * co, float * aptitud_temporal) //Devuelve el total de la aptitud_temporal
{
  float total=0.00;
  int maximo = co->cjto.back().aptitud_promedio;
  int minimo = co->cjto.front().aptitud_promedio;
  int i=0;
  //cout<<"Aptitudes Temporales!"<<endl;
  for (vector<calibracion>::iterator ca = co->cjto.begin (); ca != co->cjto.end (); ++ca)
  {
    //calculo de la aptitud temporal de cada individuo de la poblacion
    if(minimizar==1)
        aptitud_temporal[i] = minimo + maximo - (ca->aptitud_promedio); 
    else
        aptitud_temporal[i] = ca->aptitud_promedio;
    total += aptitud_temporal[i];
    //cout<<"aptitud_temporal[i]:"<< aptitud_temporal[i]<<endl;
    i++;
  } 
  //getchar();
  return total;
}

//Es mejor a que b?
bool mejor(calibracion a, calibracion b)
{ 
  if(minimizar==1)
      if((a.aptitud_promedio) < (b.aptitud_promedio)) return true;
      else return false;
  else
      if((a.aptitud_promedio) > (b.aptitud_promedio)) return true;
      else return false;
}

void agregar_calibracion_cruzada_y_calibracion_mutada(conjunto * co)
{
  int sel;
  int M=co->cjto.size(); //Conservar el mismo tamaño al final del proceso de transformacion
  poblacion_intermedia.cjto.clear();
  float aptitud_temporal[M]; //en caso de que se trate de un problema de minimizacion
  float aptitud_acumulada = calcular_aptitud_temporal(co, aptitud_temporal);
  list <int> valores_presentes;
  if(debug) cout<<"Cruzamiento"<<endl; 
  //---------------Cruzamiento-----------------
  calibracion calibracion_cruzada;
  for(int p=0; p<cantidad_parametros; p++){
    if(aptitud_acumulada>0){
      sel=seleccionar_valor_parametro_por_ruleta(co, aptitud_temporal, aptitud_acumulada, p); 
    }     
    else{
      sel=seleccionar_valor_parametro(co, p, M);      
    }
    calibracion_cruzada.parametro.push_back(sel);
  }
  calcular_aptitud_calibracion(&calibracion_cruzada);
  poblacion_intermedia.cjto.push_back(calibracion_cruzada);
  if(debug){
    cout<<"calibracion_cruzada"<<endl;
    cout<<calibracion_cruzada<<endl;
    cout<<"calidad-->" << calibracion_cruzada.aptitud_promedio<<endl;
  }
    
  //---------------Mutacion-----------------
  calibracion calibracion_mutada;
  for(int p=0; p<cantidad_parametros; p++){
    calibracion_mutada.parametro.push_back(calibracion_cruzada.parametro[p]);
  }
  calibracion_mutada=calibracion_cruzada;
  //Seleccionar parametro a mutar
  int mut=int_rand(0, cantidad_parametros);
  int vecino=0;
  if(debug) cout<<"Mutando el parametro: "<<mut<<endl;
  do{
    calibracion_mutada.parametro[mut] = int_rand(configuracion_parametros[mut].limite_minimo, (configuracion_parametros[mut].limite_maximo + 1));
    // fijar aptitud de calibración mutada en 0 (nueva cal)
    calibracion_mutada.aptitud_promedio = 0;
    calcular_aptitud_calibracion(&calibracion_mutada);
    if(debug){
      cout<<"vecino "<<vecino<<endl;
      cout<<calibracion_mutada<<endl;
    }
    vecino++;
    if(debug) cout<<"Intento "<< vecino-1<<": Comparando calibracion mutada ("<<calibracion_mutada.aptitud_promedio << ") con calibracion_cruzada (" <<calibracion_cruzada.aptitud_promedio<<")"<<endl;
  }while((!mejor(calibracion_mutada, calibracion_cruzada)) && (vecino < configuracion_parametros[mut].t_dominio));

  if(mejor(calibracion_mutada, calibracion_cruzada))
  {
    poblacion_intermedia.cjto.push_back(calibracion_mutada);
    if(debug){
      cout<<"Agregando calibracion_mutada a la poblacion "<<endl;
      cout<<calibracion_mutada<<endl;
    }
  }
  /*****************************************************/
  //copiar los restantes en conjunto_calibraciones_intermedio

  int c=poblacion_intermedia.cjto.size();
  //int tam= co->cjto.size();
  vector <calibracion>::iterator ca=co->cjto.end()-1;
  calibracion aux = *ca;
  //cout << "calidad de la peor en la pop" << aux.aptitud_promedio << endl;
  //cout<<calibracion_cruzada<<endl;
  //cout<<"calidad calibracion cruzada-->" << calibracion_cruzada.aptitud_promedio<<endl;
  CantEvalsDiscardedMut += (vecino-1)*NumSeeds;
  //cout << "perdimos esta cantidad de evaluaciones:" << (vecino-1)*NumSeeds << endl;
  if(minimizar == 1){
      if(aux.aptitud_promedio < calibracion_cruzada.aptitud_promedio) CantEvalsCrossBad += NumSeeds;
  }
  else{
      if(aux.aptitud_promedio > calibracion_cruzada.aptitud_promedio) CantEvalsCrossBad += NumSeeds;
  }

  //getchar();
  for(vector <calibracion>::iterator ca=co->cjto.begin(); ca!=co->cjto.end(), c<M; ++ca, c++){
   poblacion_intermedia.cjto.push_back(*ca);
  }
  co->cjto.clear();
  co->cjto=poblacion_intermedia.cjto;
  if(debug) cout<<endl<<"Transformacion terminada!!"<<endl;
  return;
}


bool existe_archivo(const char *fileName){
    ifstream infile(fileName);
    return infile.good();
}

int leer_archivo_candidatas(conjunto *co, int tamano){
  
  int candidatas=0;
  ifstream in(archivo_candidatas);
  string s;
  while(getline(in, s)) 
    if (!(s.compare(" ") == 0))
        candidatas++;
    
  cout<<"Hay "<< candidatas << " soluciones candidatas en el archivo." << endl;
  if(candidatas > tamano){
      cout<<"Se usaran sólo las "<< tamano << " primeras." << endl;
      candidatas = tamano;
  }
  in.close();
  
  FILE *archivo;
  archivo=fopen(archivo_candidatas, "r");
  
  //configuracion c_temp;
  
  for(int j=0; j<candidatas; j++){
    for(int i=0; i< cantidad_parametros; i++){
      if(configuracion_parametros[i].decimales == 1){ // Entero o categorico
          int v;
          fscanf(archivo, "%d", &v);
          if((v > configuracion_parametros[i].limite_maximo) || (v < configuracion_parametros[i].limite_minimo)) {
            cout << "Valor: " << v << " fuera del rango de valores especificado para: " << configuracion_parametros[i].nombre << endl;
            salir();
          }
          else
            co->cjto[j].parametro[i] = v;
      }
      else{ //Real
          float v;
          fscanf(archivo, "%f", &v);
          if(((int)(v * configuracion_parametros[i].decimales) > configuracion_parametros[i].limite_maximo) || ((int)(v * configuracion_parametros[i].decimales) < configuracion_parametros[i].limite_minimo)) {
            cout << "Valor: " << v << " fuera del rango de valores especificado para: " << configuracion_parametros[i].nombre << endl;
            salir();
          }
          co->cjto[j].parametro[i] = (int)(v * max_decimales);
          
      }
    }
  }
  fclose(archivo);
  if(debug) printf("leer_archivo_candidatas done!\n");
  return candidatas;
}

void crear_conjunto_inicial(conjunto *co, int tamano){
    
  for (int c=0; c<tamano; c++){
    calibracion ca;
    for(int p=0; p<cantidad_parametros; p++)
        ca.parametro.push_back(0);
    co->cjto.push_back(ca);
  }
  int candidatas;
  //180119
  //Verificar archivo de configuraciones candidatas
  if( existe_archivo(archivo_candidatas )){
        cout << "Leyendo configuraciones candidatas" << endl;
        candidatas=leer_archivo_candidatas(co, tamano);
  }  
  
  int tamanoi=candidatas;
  
  tamano = tamano - tamanoi;
  
  if (tamano > 0){
    //180119
    list <int> valores_presentes;
  
    for(int p=0; p<cantidad_parametros; p++){
        valores_presentes.clear();
        configuracion_parametros[p].t_dominio = configuracion_parametros[p].limite_maximo - configuracion_parametros[p].limite_minimo + 1;  
        if (configuracion_parametros[p].t_dominio > tamano){
            if(debug) cout<<"Parametro: "<<p<<", necesita mas valores que "<<tamano<<endl;
            if(configuracion_parametros[p].decimales > 1){
                float vd=((float)(max_decimales))/configuracion_parametros[p].decimales;
                valores_presentes.push_back(configuracion_parametros[p].limite_maximo*vd);
                valores_presentes.push_back(configuracion_parametros[p].limite_minimo*vd);
                int gap = configuracion_parametros[p].t_dominio / tamano;
                int current = configuracion_parametros[p].limite_minimo;
                for(int c=2; c<tamano;c++){
                    current=current+gap;
                    valores_presentes.push_back(current*vd);
                }
            }
            else{
                valores_presentes.push_back(configuracion_parametros[p].limite_maximo);
                valores_presentes.push_back(configuracion_parametros[p].limite_minimo);
                int gap = configuracion_parametros[p].t_dominio / tamano;
                int current = configuracion_parametros[p].limite_minimo;
                for(int c=2; c<tamano; c++){
                    current=current+gap;
                    valores_presentes.push_back(current);
                }
            }
            configuracion_parametros[p].t_dominio=tamano;
        }
        else{
            if(debug) cout<<"Parametro: "<<p<<", total_valores: "<<configuracion_parametros[p].t_dominio<<endl;
            for(int c=0; c<tamano; c++){
                //if(debug) cout<<"Calibracion "<<c<<endl;
                if(configuracion_parametros[p].decimales > 1){
                    int vi=(configuracion_parametros[p].limite_minimo + c%configuracion_parametros[p].t_dominio);
                    float vd=((float)(max_decimales))/configuracion_parametros[p].decimales;
                    int valor=(int)(vi*vd);
                    //if(debug) cout<<"Vi: "<< vi << ", Vd: "<<vd<<" Vf: "<<valor<<endl;
                    valores_presentes.push_back(valor);
                }
                else{
                    int valor=(configuracion_parametros[p].limite_minimo + c%configuracion_parametros[p].t_dominio);
                    //if(debug) cout<<"Valor: "<<valor<<endl;
                    valores_presentes.push_back(valor);
                }
            }
        }
        if(debug){
            for(list <int>::iterator i=valores_presentes.begin(); i!=valores_presentes.end(); ++i)
                cout<< *i << " ";
            cout<<endl;
        }
        for(int c=0; c<tamano; c++){
            int sel = int_rand(0, valores_presentes.size());
            list <int>::iterator pos;
            pos=valores_presentes.begin();
            advance(pos, sel);
            int entero = *pos;
            co->cjto[c+tamanoi].parametro[p] = entero;
            valores_presentes.erase(pos);
        }
    }
  }
  for(int p=0; p<cantidad_parametros; p++){
    if(configuracion_parametros[p].decimales > 1){
      float vd=((float)(max_decimales))/configuracion_parametros[p].decimales;
      configuracion_parametros[p].decimales = max_decimales;
      configuracion_parametros[p].limite_minimo = (int)(configuracion_parametros[p].limite_minimo*vd);
      configuracion_parametros[p].limite_maximo = (int)(configuracion_parametros[p].limite_maximo*vd);
    }
  }
  if(debug) cout<<*co;
  calcular_aptitud_conjunto_inicial(co);
  if(debug) cout<<*co;
    
  if(debug) printf("crear_conjunto_inicial done!\n");
  return;
}

void transformar(conjunto *co)
{
  sort(co->cjto.begin(), co->cjto.end(), mejor);
  //co->ordenar();
  if(debug) cout<<*co;
  
  agregar_calibracion_cruzada_y_calibracion_mutada(co);
  
  if(debug) cout<<*co;
  return;
}

int calcular_M(void){
  int Maximo = 0;
  for(int i=0; i<cantidad_parametros; i++){
    if((configuracion_parametros[i].limite_maximo - configuracion_parametros[i].limite_minimo) > Maximo)
      Maximo = configuracion_parametros[i].limite_maximo - configuracion_parametros[i].limite_minimo;
  }
  //Version 130520 Tamaño de poblacion maximo
  if (Maximo > MaxM)
    Maximo = MaxM;
  //Version 140104 Tamaño de poblacion minimo
  if(Maximo < 10)
    Maximo = 10; 
  printf("calcular_M done!\n");
  return Maximo;
}

int contar_instancias_training(char * archivo){
  int lineas=0;
  string s;
  ifstream infile;
  infile.open(archivo); // open file 
  while(infile){
    getline(infile,s);
    //cout<<"Largo linea: "<<s.length()<<endl;
    //getchar();
    if(s.length()==0) return lineas;
    else lineas++;
  }
  return lineas;
}


//hebra maestra
int main(int argc, char *argv[]) {   
    if (argc != 12) {
        cerr << "se debe tener 11 parametro y se tiene " << argc << endl;
        return 1;
    }
    // Tomar tiempo de inicio    
    global_start = chrono::high_resolution_clock::now();
    // parametros entregados
    algoritmoObjetivo = argv[1];    // ejecutable (sh) del algoritmo objetivo
    archivo_configuracion = argv[2];  // dominios iniciales de parametros y precision
    archivo_instancias = argv[3];  // path del directorio donde se encuentran las instancias
    SEED = atoi(argv[4]);               // semilla para inicialiar los números aleatorios
    NumSeeds = atoi(argv[5]);           // cantidad de semillas/instancias a probar (K semilla aleatoria)
    MaxM = atoi(argv[6]);               // tamaño max de la poblacion
    MAX_EVAL = atoi(argv[7]);           // max cantidad de evaluaciones
    MAX_TIME = atoi(argv[8]);           // max tiempo de ejecucion
    minimizar = atoi(argv[9]);          // 1=minimizar, 0=maximizar
    archivo_candidatas = argv[10];    // archivo de soluciones candidatas inciales 
    isParallel = atoi(argv[11]);       // 1=paralelo, 0=secuencial

    cout<<"------------------------------------------------------------------------"<<endl;
    cout<<"---------------------------- INICIALIZACION ----------------------------"<<endl;
    cout<<"------------------------------------------------------------------------"<<endl;
    
    // inicializar la semilla para los números aleatorios
    srand48(SEED);

    // inicializar configuraciones (dominios iniciales)
    leer_archivo_configuracion();

    // cacular popsize
    int Popsize = calcular_M();
    
    // criterio de termino
    int MAX_ITER;
    if(MAX_EVAL!=0) MAX_ITER=MAX_EVAL;
    if(MAX_TIME!=0) MAX_ITER=MAX_TIME*10;

    // inicializar lista de semillas/instancias de prueba
    CIT = contar_instancias_training(archivo_instancias);
    for(int i=0; i<MAX_ITER; i++) { // suficientes para todas las iteraciones (vida del proceso)
        cout<<"Agregando semilla/instancia: "<<i<<endl;
        agregar_semilla_instancia(&lista_semillas_instancias);
    }

    //inicializar población
    crear_conjunto_inicial(&poblacion, Popsize);


    if(debug) cout<<poblacion;

    for(I = 0; I < MAX_ITER; I++){
      cout<<"------------------------------------------------------------------------"<<endl;
      cout<<"----------------- ITERACION : "<< I <<" -------------------------------------"<<endl;
      cout<<"------------------------------------------------------------------------"<<endl;
      cout<<"-----------------AGREGANDO NUEVAS CALiBRACIONES ------------------------"<<endl;
      cout<<"------------------------------------------------------------------------"<<endl;
      transformar(&poblacion);
    }
    cout<<"Max Iteraciones!"<<I<<endl;
    
    
    salir();
    return 0; 
}
