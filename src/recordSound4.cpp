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

using namespace std;

#define SAMPLE_NUM 10000

static const int CHANNEL = 0;	// Channel of SPI (Depending on how it is connected)

void* sound( void* arg );
void* getSpiData( void* arg );
void endMelody(void);
int SpiSetup(void);

bool shutdown = true;

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
		pthread_create( &communication, NULL, &getSpiData, NULL );
	}

	while(shutdown)
	{
		delay(100);
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
			delay (1);// neco menim
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
