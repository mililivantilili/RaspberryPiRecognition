//============================================================================
// Name        : recordSound2.cpp
// Author      : Matej Kaloc
// Version     :
// Copyright   : ewerithing is not allowed
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdio.h>
#include <wiringPi.h>
#include <pthread.h>
#include <wiringPiSPI.h>
#include <fstream>

#include <signal.h>
#include <string.h>
#include <sys/time.h>

using namespace std;

#define SAMPLE_NUM 100000
#define REPRO 25
#define LED 21
#define GET_DATA 1
#define SET_RES 2
#define SPI_SETUP 3
#define START_GET_DATA 4
#define END_GET_DATA 5
#define DEFOULT 6
#define SAVE_DATA 7
#define CS0 10
#define CS1 11
#define CS2 3
#define T_us 125

static const int CHANNEL = 0;	// Channel of SPI (Depending on how it is connected)

void* sound( void* arg );
void* manageThread( void* arg );
void endMelody(void);
int SpiSetup();
void GetSpiData(void);
int SetRes(int value);
void InicializeGetData();
void UninicializeGetData();
void SaveData();
void ButtonInterrupt();

bool shutdown = true;

long count = 0;
int stav = SPI_SETUP;

unsigned char buffer[2];
//buffer[0] = 16;
unsigned short result[SAMPLE_NUM];

struct timeval current;
struct timeval start;
struct timeval startForMeasurment;
long us = T_us;
long us2 = T_us;

/*void timer_handler (int signum)
{

	count += 1;
	if (count >= 100)
	{
		result[count] = 0;
		struct timeval ts;
		gettimeofday(&ts, NULL);
		printf ("%d.%06d: timer expired %d times\n", ts.tv_sec, ts.tv_usec, count);
		count = 0;
	}
}*/

int main() {
	printf ("%s \n", "Starting program...");

	wiringPiSetup () ;
	pinMode (LED, OUTPUT) ;//svetlo
	pinMode (REPRO, OUTPUT) ;//reprak
	pullUpDnControl (1, PUD_UP) ;//wpi 1, gpio 18, nejbližší k GPIO
	wiringPiISR (1, INT_EDGE_FALLING, (*ButtonInterrupt)); //interrupt
	//for (;;)
	//{
		//digitalWrite (LED, HIGH) ;
		//digitalWrite (REPRO, HIGH) ;
		//delay (1000) ;
	    //digitalWrite (LED,  LOW) ;
	   //digitalWrite (REPRO,  LOW) ;
	   // delay (2) ;
	//}

	pthread_t sound_thread, communication;
	pthread_create( &sound_thread, NULL, &sound, NULL );

	//SPI inicializing
	/*if(SpiSetup(CHANNEL) == 0)
	{*/
		 /*struct sigaction sa;
		 struct itimerval timer;/**/
		 /* Install timer_handler as the signal handler for SIGVTALRM. */
		/* memset (&sa, 0, sizeof (sa));
		 sa.sa_handler = &timer_handler;
		 sigaction (SIGVTALRM, &sa, NULL);*/
		 /* Configure the timer to expire after 500 msec... */
		  /*timer.it_value.tv_sec = 0;
		  timer.it_value.tv_usec = 500000;*/
		  /* ... and every 500 msec after that. */
		  /*timer.it_interval.tv_sec = 0;
		  timer.it_interval.tv_usec = 500000;*/
		  /* Start a virtual timer. It counts down whenever this process is
		    executing. */
		  //setitimer (ITIMER_VIRTUAL, &timer, NULL);

		pthread_create( &communication, NULL, &manageThread, NULL );

	/*}*/



	while(shutdown)
	{
		//delay(100);
	}

	printf ("%s \n", "End program");
	endMelody();

	return 0;
}

void* sound( void* arg )
{
	for(int i = 0; i<50; i++)
	{
		digitalWrite (REPRO, HIGH);
		delay (4);
		digitalWrite (REPRO, LOW);
		delay (4);
	}
	for(int i = 0; i<100; i++)
	{
		digitalWrite (REPRO, HIGH);
		delay (2);
		digitalWrite (REPRO, LOW);
		delay (2);
	}
	//shutdown = false;
	return 0;
}

void endMelody(void)
{
	for(int i = 0; i<50; i++)
	{
		digitalWrite (REPRO, HIGH);
		delay (5);
		digitalWrite (REPRO, LOW);
		delay (5);
	}
	for(int i = 0; i<100; i++)
	{
		digitalWrite (REPRO, HIGH);
		delay (1);
		digitalWrite (REPRO, LOW);
		delay (1);
	}
	for(int i = 0; i<50; i++)
	{
		digitalWrite (REPRO, HIGH);
		delay (5);
		digitalWrite (REPRO, LOW);
		delay (5);
	}
}

void* manageThread( void* arg ){

	printf("manageThread started \n");

	while(shutdown){
		switch (stav)
		{
		case SPI_SETUP:
			if(SpiSetup() == 0){
				stav = SET_RES;
			}
			else{
				shutdown = false;
			}
			break;
		case START_GET_DATA:
			printf("Inicialize measurment \n");
			InicializeGetData();
			break;
		case GET_DATA:
			GetSpiData();
			break;
		case END_GET_DATA:
			printf("Uninicialize measurment \n");
			UninicializeGetData();
			break;
		case SAVE_DATA:
			printf("Saving data \n");
			SaveData();
			break;
		case SET_RES:
			printf("Seting resistor \n");
			SetRes(128);
			stav = START_GET_DATA;
			break;
		default:
			shutdown = false;
			delay(50);
			break;
		}
	}

	return 0;
}

int SpiSetup(){
	if(wiringPiSetup() == -1)
	{
		printf("%s \n", "Unable to start wiringPi");
		return 1 ;
	}

	if(wiringPiSPISetup(CHANNEL, 200000) == -1)
	{
		printf("%s \n", "wiringPiSPISetup Failed");
		return 1 ;
	}

	pinMode(CS0, OUTPUT);	// 8 is GPIO number of cs of ADC
	pinMode(CS1, OUTPUT);   // 7 is GPIO number of cs of MIC1
	pinMode(CS2, OUTPUT);   // 7 is GPIO number of cs of MIC1

	return 0;
}

void GetSpiData(void){

	buffer[0] = 4;	// 8 is channel 2, 4 is channel 1
	buffer[1] = 0;
	wiringPiSPIDataRW(CHANNEL, buffer, 2);
	result[count%SAMPLE_NUM] = (buffer[0] << 8) + buffer[1];


	count ++;
	if (count >= SAMPLE_NUM)
	{
		printf ("Number of values: %ld \nNumber of useconds: %ld \n" ,count, (start.tv_usec + start.tv_sec*1000000)-(startForMeasurment.tv_usec + startForMeasurment.tv_sec*1000000));
		count = 0;
		stav = END_GET_DATA;
	}

	do {
		gettimeofday(&current, NULL);
	}
	while( ( current.tv_usec + current.tv_sec*1000000 ) - ( start.tv_usec + start.tv_sec*1000000 ) <= us2 );

	us2=2*us-(( current.tv_usec + current.tv_sec*1000000 ) - ( start.tv_usec + start.tv_sec*1000000 ));
	start = current;
}

void InicializeGetData(){

	digitalWrite(CS0, 0);  // Low : CS Active
	printf("pin 10 setted \n");

	stav = GET_DATA;

	gettimeofday(&start, NULL);
	startForMeasurment = start;
}

void UninicializeGetData(){
	digitalWrite(CS0, 1);  // Low : CS Inactive
	stav = SAVE_DATA;
}

void SaveData(){

	std::ofstream output_file("/home/pi/test.txt");				//Will overwrite an existing file
	output_file << result[0];
	output_file << " ";
	//printf("%s %i \n", "Result[0] =", result[0]);
	output_file.close();

	std::ofstream output_file2("/home/pi/test.txt", std::ios_base::app);	//Will append to an existing file
	for(int i = 1; i < SAMPLE_NUM; i++){
		output_file2 << result[i];
		output_file2 << " ";
	}
	output_file2.close();
	//output_file.close();			//(Not strictly necessary as it will get closed when output_file goes out of scope)
	//shutdown = false;

	stav = DEFOULT;
}

int SetRes(int value){
	digitalWrite(CS1, 0); // SC of MIC1 set active
	delay(1);
	buffer[0] = 0b00000000;	// 48 pro psaní, + dva lsb pro nejvyšší data
	buffer[1] = (value > 255) ? 255 : value;
	wiringPiSPIDataRW(CHANNEL, buffer, 2);
	delay(10);
	int resultOfSet = (buffer[0] << 8) + buffer[1];
	printf ("%d\n", resultOfSet);
	digitalWrite(CS1, 1); // SC of MIC1 set inactive

	return 1;
}

void ButtonInterrupt(){
	printf ("%s \n", "Funguju");
}
