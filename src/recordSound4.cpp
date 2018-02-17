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

#define SAMPLE_NUM 10000

static const int CHANNEL = 0;	// Channel of SPI (Depending on how it is connected)

void* sound( void* arg );
void* getSpiData( void* arg );
void endMelody(void);
int SpiSetup(void);

bool shutdown = true;

unsigned short result[SAMPLE_NUM];

static int count = 0;

void timer_handler (int signum)
{
	result[count] = 0;
	count += 1;
	printf ("Timer expired %d times\n", count);
}

int main() {
	printf ("%s \n", "Starting program...");

	wiringPiSetup () ;
	pinMode (6, OUTPUT) ;//svetlo
	pinMode (7, OUTPUT) ;//reprak
	//for (;;)
	//{
		//digitalWrite (6, HIGH) ;
		//digitalWrite (7, HIGH) ;
		//delay (2) ;
	    //digitalWrite (6,  LOW) ;
	   //digitalWrite (7,  LOW) ;
	   // delay (2) ;
	//}

	pthread_t sound_thread, communication;
	pthread_create( &sound_thread, NULL, &sound, NULL );

	//SPI inicializing
	if(SpiSetup() == 0)
	{
		 struct sigaction sa;
		 struct itimerval timer;
		 /* Install timer_handler as the signal handler for SIGVTALRM. */
		 memset (&sa, 0, sizeof (sa));
		 sa.sa_handler = &timer_handler;
		 sigaction (SIGVTALRM, &sa, NULL);
		 /* Configure the timer to expire after 500 msec... */
		  timer.it_value.tv_sec = 0;
		  timer.it_value.tv_usec = 500000;
		  /* ... and every 500 msec after that. */
		  timer.it_interval.tv_sec = 0;
		  timer.it_interval.tv_usec = 500000;
		  /* Start a virtual timer. It counts down whenever this process is
		    executing. */
		  setitimer (ITIMER_VIRTUAL, &timer, NULL);
		/*
		pthread_create( &communication, NULL, &getSpiData, NULL );
		*/
	}



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
		digitalWrite (7, HIGH);
		delay (4);
		digitalWrite (7, LOW);
		delay (4);
	}
	for(int i = 0; i<100; i++)
	{
		digitalWrite (7, HIGH);
		delay (2);
		digitalWrite (7, LOW);
		delay (2);
	}
	//shutdown = false;
	return 0;
}

void endMelody(void)
{
	for(int i = 0; i<50; i++)
	{
		digitalWrite (7, HIGH);
		delay (5);
		digitalWrite (7, LOW);
		delay (5);
	}
	for(int i = 0; i<100; i++)
	{
		digitalWrite (7, HIGH);
		delay (1);
		digitalWrite (7, LOW);
		delay (1);
	}
	for(int i = 0; i<50; i++)
	{
		digitalWrite (7, HIGH);
		delay (5);
		digitalWrite (7, LOW);
		delay (5);
	}
}

void* getSpiData( void* arg ){
	while(true){
		unsigned char buffer[2];
		buffer[0] = 16;
		unsigned short result[SAMPLE_NUM];
		for(int i = 0; i <SAMPLE_NUM; i++)
		{
			digitalWrite(8, 0);  // Low : CS Active
			buffer[0] = 4;	// 8 is channel 2, 4 is channel 1
			buffer[1] = 0;
			wiringPiSPIDataRW(CHANNEL, buffer, 2);
			result[i] = (buffer[0] << 8) + buffer[1];
			digitalWrite(8, 1);  // Low : CS Inactive
			delay (1);
		}
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
		delay(1000);
	}
	return 0;
}

int SpiSetup(void){
	if(wiringPiSetup() == -1)
	{
		printf("%s \n", "Unable to start wiringPi");
		return 1 ;
	}

	if(wiringPiSPISetup(CHANNEL, 500000) == -1)
	{
		printf("%s \n", "wiringPiSPISetup Failed");
		return 1 ;
	}

	pinMode(8, OUTPUT);	// 8 is GPIO number of cs of ADC

	return 0;
}
