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
#include <stdlib.h>

#include <signal.h>
#include <string.h>
#include <sys/time.h>

//TCP server
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
//TCP server

using namespace std;

// 10 sekund při 50ksps
#define SAMPLE_NUM 500000
#define REPRO 25
#define LED 21
//WPI numbers of my DIO
#define DIO1 1
#define DIO2 7
#define DIO3 0
#define DIO4 2

#define GET_DATA 1
#define SET_RES 2
#define SPI_SETUP 3
#define START_GET_DATA 4
#define END_GET_DATA 5
#define DEFOULT 6
#define SAVE_DATA 7
#define NEW_WORD 8
#define GET_DC 9
#define CS0 10
#define CS1 11
#define CS2 3
//#define T_us 125
// 50ksps
#define T_us 20
#define SAVED_DATA_NAME "test"
#define OKNO_KRATKODOBE_INTENZITY 160
//TCP server
#define TCP_PORT 51717;
//'TCP server

static const int CHANNEL = 0;	// Channel of SPI (Depending on how it is connected)

void* sound( void* arg );
void* manageThread( void* arg );
void* makeImpulseThread(void* arg);
void* TcpServerThread(void* arg);
void endMelody(void);
int SpiSetup();
void GetSpiData(void);
int SetRes(int value);
void InicializeGetData();
void UninicializeGetData();
void SaveData(string DataName);
void ButtonInterrupt();
void RecordingNewWord();
void DcEstimation();
void CalculateDc();


bool Shutdown = true;
bool recordingNewWord = false;

long count = 0;
int stav = SPI_SETUP;
int pin = 0;

unsigned char buffer[2];
//buffer[0] = 16;
signed short result[SAMPLE_NUM];
signed long kratkodoba_intenzita[SAMPLE_NUM];

struct timeval current;
struct timeval start;
struct timeval startForMeasurment;
long us = T_us;
long us2 = T_us;
unsigned int dc = 0;

pthread_t outputThread;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockDIO1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockDIO2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockDIO3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockDIO4 = PTHREAD_MUTEX_INITIALIZER;

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
	pullUpDnControl (DIO1, PUD_UP) ;//wpi 1, gpio 18, nejbližší k GPIO
	wiringPiISR (DIO1, INT_EDGE_BOTH, (*ButtonInterrupt)); //interrupt na zmáčknutí

	pinMode (DIO2, OUTPUT);
	pinMode (DIO3, OUTPUT);
	pinMode (DIO4, OUTPUT);
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
		 struct itimerval timer;*/
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

		//TCP server
		pthread_t tcp_server;
		pthread_create( &tcp_server, NULL, &TcpServerThread, NULL );
		// TCP server

	/*}*/



	while(Shutdown)
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
	//Shutdown = false;
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

	while(Shutdown){
		switch (stav)
		{
		case SPI_SETUP:
			if(SpiSetup() == 0){
				stav = SET_RES;
			}
			else{
				Shutdown = false;
			}
			break;
		case START_GET_DATA:
			printf("Inicialize measurment \n");
			InicializeGetData();
			stav = GET_DATA;
			break;
		case GET_DATA:
			GetSpiData();
			break;
		case END_GET_DATA:
			printf("Uninicialize measurment \n");
			UninicializeGetData();
			stav = SAVE_DATA;
			break;
		case SAVE_DATA:
			printf("Saving data \n");
			SaveData(SAVED_DATA_NAME);
			stav = DEFOULT;
			break;
		case SET_RES:
			printf("Seting resistor \n");
			//SetRes(128);
			SetRes(200);
			stav = GET_DC;
			break;
		case GET_DC:
			printf("Getting  DC \n");
			CalculateDc();
			stav = START_GET_DATA;
			break;
		case NEW_WORD:
			printf("Recording your word \n");
			RecordingNewWord();
			break;
		default:
			//Shutdown = false;
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
	result[count%SAMPLE_NUM] = (buffer[0] << 8) + buffer[1] - dc;


	count ++;
	/*if (count >= SAMPLE_NUM && !recordingNewWord)
	{
		printf ("Number of values: %ld \nNumber of useconds: %ld \n" ,count, (start.tv_usec + start.tv_sec*1000000)-(startForMeasurment.tv_usec + startForMeasurment.tv_sec*1000000));
		count = 0;
		stav = END_GET_DATA;
	}*/

	do
	{
		gettimeofday(&current, NULL);
	}
	while( ( current.tv_usec + current.tv_sec*1000000 ) - ( start.tv_usec + start.tv_sec*1000000 ) <= us2 );


	us2=2*us-(( current.tv_usec + current.tv_sec*1000000 ) - ( start.tv_usec + start.tv_sec*1000000 ));
	start = current;
}

void InicializeGetData(){

	digitalWrite(CS0, 0);  // Low : CS Active

	gettimeofday(&start, NULL);
	startForMeasurment = start;
}

void UninicializeGetData(){
	digitalWrite(CS0, 1);  // Low : CS Inactive

}

void SaveData(string DataName){

	DataName = ("/home/pi/" + DataName) + ".txt";
	std::ofstream output_file(DataName.c_str());				//Will overwrite an existing file
	output_file << result[0];
	output_file << " ";
	//printf("%s %i \n", "Result[0] =", result[0]);
	output_file.close();

	std::ofstream output_file2(DataName.c_str(), std::ios_base::app);	//Will append to an existing file
	for(int i = 1; i < count; i++){
		output_file2 << result[i];
		output_file2 << " ";
	}
	output_file2.close();
	//output_file.close();			//(Not strictly necessary as it will get closed when output_file goes out of scope)
	//Shutdown = false;
}

int SetRes(int value){
	digitalWrite(CS1, 0); // SC of MIC1 set active
	delay(1);
	buffer[0] = 0b00000000;	// 0 pro psaní
	buffer[1] = (value > 255) ? 255 : value;
	wiringPiSPIDataRW(CHANNEL, buffer, 2);
	delay(10);
	int resultOfSet = (buffer[0] << 8) + buffer[1];
	printf ("%d\n", resultOfSet);
	digitalWrite(CS1, 1); // SC of MIC1 set inactive

	return 1;
}

void ButtonInterrupt(){
	printf("Interrupt \n");

	// nahrávání po zmáčknutí
		/*if(!digitalRead(1))
		{
			recordingNewWord = true;
			stav = NEW_WORD;
		}
		else
		{
			recordingNewWord = false;
		}*/

		//rozsvícení
	pthread_mutex_lock(&lock);
	pin = DIO2;
	pthread_create( &outputThread, NULL, &makeImpulseThread, NULL );

	pthread_mutex_lock(&lock);
	pin = DIO3;
	pthread_create( &outputThread, NULL, &makeImpulseThread, NULL );

	pthread_mutex_lock(&lock);
	pin = DIO4;
	pthread_create( &outputThread, NULL, &makeImpulseThread, NULL );

}

void RecordingNewWord(){
	count = 0;
	InicializeGetData();
	digitalWrite (LED, HIGH);
	while(recordingNewWord){
		GetSpiData();
	}
	digitalWrite (LED, LOW) ;
	UninicializeGetData();
	SaveData("slova/pocitaci4");
	stav = DEFOULT;//START_GET_DATA;
}

void CalculateDc(){
	count = 0;
	InicializeGetData();
	digitalWrite (LED, HIGH);
	while(count<=1000){
		GetSpiData();
	}
	digitalWrite (LED, LOW) ;
	UninicializeGetData();
	DcEstimation();
	printf ("DC: %d \n", dc);
}

void DcEstimation(){
	unsigned int OldDc = dc;
	dc = 0;

	for(int i = 500; i<1000; i++)
	{
		dc += result[i];
	}
	dc = (dc/500) + OldDc;
}

int KratkodobaIntenzita(){

	for(int i = 0; i<(count-OKNO_KRATKODOBE_INTENZITY); i++)
	{
		for(int k = i; k<(i+OKNO_KRATKODOBE_INTENZITY);k++)
		{
			kratkodoba_intenzita[i+OKNO_KRATKODOBE_INTENZITY] += abs(result[k]);
		}
	}

	return 1;
}

void* makeImpulseThread(void* arg)
{

	int threadsPin =  pin;
	pthread_mutex_unlock(&lock);

	printf ("Starting thread: %d \n", threadsPin);

	switch(threadsPin)
	{
	case DIO1:
		pthread_mutex_lock(&lockDIO1);
		break;
	case DIO2:
		pthread_mutex_lock(&lockDIO2);
		break;
	case DIO3:
		pthread_mutex_lock(&lockDIO3);
		break;
	case DIO4:
		pthread_mutex_lock(&lockDIO4);
		break;
	default:
		pthread_mutex_lock(&lockDIO1);
		threadsPin = DIO1;
		break;
	}

	digitalWrite (threadsPin, HIGH);
	delay (2000);
	digitalWrite (threadsPin, LOW);
	delay (2000);

	switch(threadsPin)
	{
	case DIO1:
		pthread_mutex_unlock(&lockDIO1);
		break;
	case DIO2:
		pthread_mutex_unlock(&lockDIO2);
		break;
	case DIO3:
		pthread_mutex_unlock(&lockDIO3);
		break;
	case DIO4:
		pthread_mutex_unlock(&lockDIO4);
		break;
	default:
		pthread_mutex_unlock(&lockDIO1);
		break;
	}


	return 0;
}

void* TcpServerThread(void* arg)
{
	//http://www.linuxhowtos.org/C_C++/socket.htm
	//int argc = TCP_PORT;
	int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
    	printf("ERROR opening socket");
    	exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = TCP_PORT;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
    	printf("ERROR on binding");
    	exit(1);
    }

    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    while(1)
    {
    	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    	if (newsockfd < 0)
    	{
    		printf("ERROR on accept");
    		exit(1);
    	}
    	bzero(buffer,256);
    	n = read(newsockfd,buffer,255);
    	if (n < 0)
    	{
    		printf("ERROR reading from socket");
    		exit(1);
    	}
    	printf("Here is the message: %s\n",buffer);

    	if (strcmp(buffer,"StartRecording") == 0)
    	{
    		if (!recordingNewWord)
    		{
    			n = write(newsockfd,"Starting to record",18);
    			recordingNewWord = true;
    			stav = NEW_WORD;
    		}
    		else
    		{
    			n = write(newsockfd,"Already recording",17);
    		}
    	}
    	else if(strcmp(buffer,"StopRecording") == 0)
    	{
    		if (recordingNewWord)
    		{
    			n = write(newsockfd,"Stopping recording",18);
    			recordingNewWord = false;
    		}
    		else
    		{
    			n = write(newsockfd,"Already stopped",15);
    		}
    	}
    	else
    	{
    		n = write(newsockfd,"Incorrect request",17);
		}


    	if (n < 0)
    	{
    		printf("ERROR writing to socket");
    		exit(1);
    	}
    }
    close(newsockfd);
    close(sockfd);
	return 0;
}
