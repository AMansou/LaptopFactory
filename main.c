/*****************************************************************************
* FILE: factory.c                                                            *
* DESCRIPTION:                                                               *
* This program is a simulation of a laptop factory. it has NUM_PL Production *
* lines. Threads are used to simulate each of the storage, driver, loading,  *
* HR and CEO employees.                                                      *
* AUTHORS: Ahmad Mansour 1172631 and Dima Younes 11740020                    *
* LAST REVISED:  26/11/21                                                    *
******************************************************************************/
/*********************************INCLUDE*********************************/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/msg.h>
#include <stddef.h>
#include <signal.h>
#include <time.h>
/*********************************Definitions*********************************/
#define NUM_PL            10        //how many production lines
#define STORAGE_SLEEP     3         //storage employee sleep time
#define NUM_STORAGE       3         //How many storage workers
#define NUM_TRUCKS	      2         //how many loading trucks.
#define CARTONS_TRIP    	2         //how many cartons carried in a trip
#define TRUCK_DELAY       40        //how long until the truck comes back
#define LP_CARTON         10        // LAptops per carton
#define SALARY_TECHNICAL  20
#define SALARY_CEO        40
#define SALARY_TRUCK      10
#define SALARY_STORAGE    10
#define SALARY_HR         10
#define COST              100     // Cost of producing a single laptop
#define PRICE             300     // Laptop price
#define TIME              50      // every TIME seconds, the workers receive their salaries.
#define HIGH_THRESH       3000    // When profit reaches HIGH_THRESH, unsuspend any suspended Production lines
#define LOW_THRESH        -1000   // Suspend production lines whenever profit goes below LOW_THRESH
#define GOAL_PROFIT       1000000 // When GOAL_PROFIT is reached, exit
#define MINSTORAGE        60      // continue producing laptops once storage is less than MINSTORAGE
#define FULLSTORAGE       100     // When storage==FULLSTORAGE, stop producing laptops.
#define TECHNICAL_WAIT    2       // A technical employee will be busy for 1-TECHNICAL_WAIT seconds with his respective task
/*********************************MUTEXES AND CONDITION VARIABLES*********************************/
pthread_cond_t trans_cond = PTHREAD_COND_INITIALIZER;  // Transportation condition variable
pthread_mutex_t mutexcnt[NUM_PL];                      // A mutex for each production line
pthread_mutex_t mutex_sold=PTHREAD_MUTEX_INITIALIZER;  // A mutex for the global variable "sold"
pthread_mutex_t mutexDec=PTHREAD_MUTEX_INITIALIZER;    // A mutex for the global variable "decision"
pthread_mutex_t storage_mutex[1];                      // Storage mutex
pthread_cond_t storage_cond = PTHREAD_COND_INITIALIZER;//storage condition variable
pthread_cond_t dec_cond = PTHREAD_COND_INITIALIZER; // condition variable to let CEO know of any need to suspend an employee
/*********************************GLOBAL VARIABLES*********************************/
int decision=1;                                     // this is changed according to profit. 1 == no action, 2 unsuspend, 0 suspend.
int transFlag[NUM_TRUCKS],storeFlag[NUM_STORAGE],techFlag[NUM_PL];  // Each worker will keep working until its flag stops being 1
int progress[NUM_PL];                                               // For each production line, there is a progress variable to keep track of which step to be executed next
short done[10][NUM_PL];                                             // A step should not be repeated more than once for every laptop. should be reset everytime a PL finishes a laptop.
int sold=0;                                                         // Keep track of how many laptops sold in a period of time
int plbox[NUM_PL] ;                                                 //a box for every production line
int storagec = 0;                                                   // storage counter
/*********************************FUNCTION PROTOTYPES*********************************/
int protecc(int, int, char *);
void *technical(void *);
void *production(void *);
void *storage(void *);
void *transportation(void * );
void *ceoThread();
void *hrThread();
/*********************************STRUCTS*********************************/
typedef struct
{
    int pl;
    int task;
} ID;           // Each technical worker has a task and a production line
/*********************************MAIN*********************************/
int main(int argc, char *argv[])
{
    pthread_t prod_line[NUM_PL]; // to create threads by the number of production lines
    pthread_t TRUCK[NUM_TRUCKS];// to create threads by the number of production lines
    pthread_t storageThread[NUM_PL];
    pthread_attr_t attr;         // to make a thread joinable
    pthread_t ceo;
    pthread_t hr;
    void *status;
    /*Initialize needed variables*/
    pthread_mutex_init(&storage_mutex[0], NULL);
    for(int i=0;i<10;i++)
        for(int j=0;j<NUM_PL;j++)
            done[i][j]=0;
    for(int i=0;i<NUM_PL;i++)
    {
        progress[i]=0;
        plbox[i]=0;
        techFlag[i]=1;
        /*
        *initialize the mutexes. One mutex for each PL.
        */
        pthread_mutex_init(&mutexcnt[i], NULL);
    }
    for(int i=0;i<NUM_STORAGE;i++)
      storeFlag[i]=1;
    for(int i=0;i<NUM_TRUCKS;i++)
      transFlag[i]=1;
    /* Initialize and set thread detached attribute */
    protecc(pthread_attr_init(&attr), 0, "attr_init");
    protecc(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE), 0, "setdetach");
    /*
     * creating thread for each production line
     */
    for (int t = 0; t < NUM_PL; t++)
    {
        printf("Main: creating thread %ld\n", t);
        protecc(pthread_create(&prod_line[t], &attr, production, (void *)t), 0, "Thread create");
    }
    /*
     * creating thread for each storage employee
     */
    for (int i = 0; i < NUM_STORAGE; i++)
    {
        printf("Main: creating storage simp %ld\n", i);
        protecc(pthread_create(&storageThread[i], &attr, storage, (void *)i), 0, "Thread create");
    }
    /*
     * creating thread for each truck employee
     */
    for(int t=0; t<NUM_TRUCKS; t++) //create a thread for each production line
    {
        printf("Main: creating truck %ld\n", t);
        protecc(pthread_create(&TRUCK[t], &attr, transportation, (void *)t),0,"Thread create");
    }
    /*
     * creating threads for HR and CEO
     */
    protecc(pthread_create(&ceo, &attr, ceoThread,NULL),0,"Thread create");
    protecc(pthread_create(&hr, &attr, hrThread,NULL),0,"Thread create");
    /* Free attribute and wait for the other threads */
    pthread_attr_destroy(&attr);

    pthread_exit(NULL); //wait til all threads are done
}
/*Used to detect function errors*/
int protecc(int ret, int no, char *message)
{
    if (ret)
    {
        perror(message);
        exit(no);
    }
    else
        return ret;
}
/*********************************THREADS*********************************/
/*
 *each produc. line will create threads according to how many tasks there are (10 tasks)
 */
void *production(void *t)
{
    int num_steps = 10;
    pthread_attr_t attr;
    pthread_t step[num_steps];
    void *status;
    int tid = (int)t;
    ID *s[10]; // we need to pass the id of each thread to it as an argument
    /* Initialize and set thread detached attribute */
    protecc(pthread_attr_init(&attr), 0, "attr_init");
    protecc(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE), 0, "setdetach");
    /*
     * creating thread for each step in each production line
     */
    for (int i = 0; i < num_steps; i++)
    {
        s[i] = malloc(sizeof(ID));
        s[i]->pl = tid;
        s[i]->task = i;
        protecc(pthread_create(&step[i], &attr, technical, s[i]), 0, "Thread create");
    }
    pthread_attr_destroy(&attr);
    pthread_exit((void *)t);
}
/*********************************TECHNICAL*********************************/
void *technical(void *t)
{
    ID *s;
    s = (ID *)t;
    srand(time(NULL) + s->pl + s->task);
    /* Initialize and set thread detached attribute */
    sleep(2);
    while (techFlag[s->pl])
    {
        /***************************************************************************************************
         * To make sure the first five tasks are run in order, we check if the task# maches the progress
         * However, once progress has reached the 5th task, any task above4 will do
         * Of course, don't forget to check if the task has already been done
         ***************************************************************************************************/
        pthread_mutex_lock(&mutexcnt[s->pl]); // Each technical employee should lock its respective prod. line mutex.
        if ((s->task == progress[s->pl] || progress[s->pl] > 4 && s->task > 4) && !done[s->task][s->pl])  // If task==progress in pl or both progress and task >4
        {                                                                                                 // And the task wasn't done yet
            printf("task #%ld, Progress: %d. In production line %d\n", s->task, progress[s->pl],s->pl);
            if (progress[s->pl] == 9) //At the last task
            {
                progress[s->pl] = 0; //Reset the progress
                sleep(1);
                printf("Laptop is finished\n");
                for (int i = 0; i < 10; i++)
                {
                    done[i][s->pl] = 0; //Reset the done flag in all of the PL
                }
                pthread_mutex_lock(&storage_mutex[0]);
                plbox[s->pl]++; //Increase the number of laptops produced by this PL
                printf("Production Line #%d has %d laptops\n", s->pl, plbox[s->pl]);
                /*
                 * here we wait for the each production line to produce 10 laptops (condition)
                 * so the storage employee can store them in the storage room
                 */
                if (plbox[s->pl] >= LP_CARTON)
                    /*
                     *wakes up any thread waiting for this condition
                     */
                    pthread_cond_signal(&storage_cond);

                pthread_mutex_unlock(&storage_mutex[0]);
            }
            else
            {
                progress[s->pl]++;        //Next task
                done[s->task][s->pl] = 1; //Flag the task as done
            }
            sleep(1+rand() % TECHNICAL_WAIT); //random time for task
            printf("%d is done in PL %d\n", s->task, s->pl);
            pthread_mutex_unlock(&mutexcnt[s->pl]);
        }
        else
          pthread_mutex_unlock(&mutexcnt[s->pl]);
    }
    pthread_exit(NULL);
}
/*********************************STORAGE*********************************/
void *storage(void *t)
{
    while(storeFlag[(int)t])
    {
        /*********************************************************************************
         * the storage emp. waits for the box to have 10 laptops
         * once 10 laptops are placed, each production line starts producing back laptops
         * and storage empoyee stores laptops in storagec
         *********************************************************************************/
        pthread_mutex_lock(&storage_mutex[0]);
        pthread_cond_wait(&storage_cond, &storage_mutex[0]); // wainting for the signal from technical
        /*
         * moving carton of laptops from production line to the storage room
         */
        for(int i=0; i<NUM_PL; i++)
        {
            if(plbox[i]>=LP_CARTON)
            {
                plbox[i] -= LP_CARTON;
                storagec += LP_CARTON;
            }
        }
        printf("STORAGE EMPLOYEE #%d IS LOADING NOW, TOTAL = %d LAPTOPS\n", pthread_self(), storagec);
        pthread_mutex_unlock(&storage_mutex[0]);
        /*
         * Once storage is full, lock every PL mutex (mutexcnt) and wait until storage is less than MINSTORAGE
         */
        if(storagec>FULLSTORAGE)
        {
          for(int i=0;i<NUM_PL;i++)
            pthread_mutex_lock(&mutexcnt[i]);  //lock all PLs
          while (storagec>=MINSTORAGE);       // busy wait
          for(int i=0;i<NUM_PL;i++)
          pthread_mutex_unlock(&mutexcnt[i]);// unlock all pls
        }
        sleep(STORAGE_SLEEP);
    }
    pthread_exit(NULL);
}
/*********************************TRANSPORTATION*********************************/
void *transportation(void *t)
{
    int load=CARTONS_TRIP*LP_CARTON;          // How much a truck can carry

    while(transFlag[(int)t])
    {
      pthread_mutex_lock (&storage_mutex[0]); // Lock to be able to use the variable "storagec"
      if(storagec>=load)                      // If there are enough cartons to load into truck
      {
        storagec-=load;                       // Move the cartons from storage into the truck
        pthread_mutex_lock (&mutex_sold);
        sold+=load;                           // increase the number of laptops sold
        pthread_mutex_unlock (&mutex_sold);
        printf("TRANSPORTED!  Storage remaining: %d\n",storagec);
      }
      pthread_mutex_unlock (&storage_mutex[0]);
      sleep(TRUCK_DELAY);
    }

}
/*********************************CEO*********************************/
void *ceoThread()
{
  int prod=0;                   // to keep track of which lines are suspended
  pthread_t prod_line;         // To unsuspend a thread, recreate it
  pthread_attr_t attr;         // to make a thread joinable
  /* Initialize and set thread detached attribute */
  protecc(pthread_attr_init(&attr), 0, "attr_init");
  protecc(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE), 0, "setdetach");
  while (1) {
    pthread_mutex_lock(&mutexDec);
    pthread_cond_wait(&dec_cond, &mutexDec);   // wainting for the signal from HR
    pthread_mutex_unlock(&mutexDec);
    if(decision==0 )                          // If the decision is to suspend a line
      {
        /*
         *  exit if 50% of lines were to be suspended
         */
        if(prod>=NUM_PL/2)
        {
            printf("\n\n\n\n\n\n\n\t\t GOING BANKRUPT . . . EXITING\n\n\n\n\n\n\n\n\n\n " );
            exit(0);
        }
        techFlag[prod++]=0;                   // Stop the infinite loop and point to the next line
      }
      else if(decision==2 && prod )           // If the decision is to unsuspend
      {
          techFlag[--prod]=1;                 // reset the flag
          protecc(pthread_create(&prod_line, &attr, production, (void *)prod), 0, "Thread create"); //Create production line again
      }
  }


}
/*********************************HR*********************************/
void *hrThread()
{
  int salaries=SALARY_CEO+SALARY_HR+SALARY_TRUCK*NUM_TRUCKS+SALARY_STORAGE*NUM_STORAGE+SALARY_TECHNICAL*NUM_PL*10;;
  int expenses;
  int profit;
  while (1) {
    sleep(TIME);
    pthread_mutex_lock (&mutex_sold);
    /*
     * Compute expenses
     */
    expenses=COST*sold+salaries;
    profit+=PRICE*sold-expenses;
    sold=0;
    pthread_mutex_unlock (&mutex_sold);
    if(profit>=GOAL_PROFIT)
      {
        printf("\n\n\n\n\n\n\n\t\t WE ARE RICH NOW SO NO NEED TO CONTINUE . . . EXITING\n\n\n\n\n\n\n\n\n\n " );
        exit(0);

      }
    /*
     * Make a decision to improve profit
     */
    if (profit<LOW_THRESH)
      {
        decision=0;                     //suspend a PL
        salaries-=SALARY_TECHNICAL*10;  // Decrese salary expense
      }
    else if(profit>HIGH_THRESH)
      {
        decision=2;                     // Unsuspend
        if(salaries+SALARY_TECHNICAL*10<=SALARY_CEO+SALARY_HR+SALARY_TRUCK*NUM_TRUCKS+SALARY_STORAGE*NUM_STORAGE+SALARY_TECHNICAL*NUM_PL*10)
          salaries+=SALARY_TECHNICAL*10;// increase salary expense
      }
    else
      decision=1;                       // DO nothing
    pthread_mutex_lock(&mutexDec);
    pthread_cond_signal(&dec_cond);     // Inform CEO of the decision.
    pthread_mutex_unlock(&mutexDec);
    printf("\n\n\n\The profit is:\n\t%d\n\n\n**********************\n",profit );
  }
}
/*******************************************************************************END*********************************************************************************************/
