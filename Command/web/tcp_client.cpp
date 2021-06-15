#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>

#include <iostream>
#include <stdint.h>
#include <string>
#include <cstdlib>

int main(int argc,char *argv[]){

	sockaddr_in saddr={AF_INET,
		htons(4000),{/*0x0100007f*//*0x1520351f*/0x606D8D51}
	};

	int s=socket(AF_INET,SOCK_STREAM,0);

	if(s < 0){
		throw std::runtime_error("Error opening socket");
	}

	if(connect(s,(sockaddr*)&saddr,sizeof(saddr))<0){
		throw std::runtime_error("Error connecting");
	}

	int num = 0;
	//calculate number to send
	std::string mode = argv[1];
	if(mode == "info"){
		num = 0b10100000000000000000;
	} else if(mode == "move"){
		num = 0b10000000000000000000;
		int amount = atoi(argv[2]);
		if(amount>=0){
			num += amount;
		} else {
			amount &= 0b1111111111111111;
			num += amount;
		}
	} else if(mode == "rotate") {
		num = 0b10010000000000000000;
		float f_amount = atof(argv[2]);
		int amount = 10 * f_amount;
		if(amount>=0){
			num += amount;
		} else {
			amount &= 0b1111111111111111;
			num += amount;
		}

	} else {
		throw std::runtime_error("Mode not supported");
	}
	//end

	std::string value = std::to_string(num) + "/";
	
	if(send(s,value.c_str(),value.size(),0)<0){
		throw std::runtime_error("Error sending");
	}
	
	//receive position of rover: x-pos 0-65535,y-pos 0-65535,rotation 0-4095,current 0-65535,speed 0-65535
	char pos[29];
	recv(s,&pos,sizeof(pos),0);
	std::cout << pos;

	//receive battery status of rover: charge capacity 0-100,charging power 0-100,range 0-65535
	char battery[16];
	recv(s,&battery,sizeof(battery),0);
	std::cout << battery;

  //receive Red object status: seen 0-1,x-pos 0-65535,y-pos 0-65535
	char object_red[16];
	recv(s,&object_red,sizeof(object_red),0);
	std::cout << object_red;

	//receive green object status: seen 0-1,x-pos 0-65535,y-pos 0-65535
	char object_green[16];
	recv(s,&object_green,sizeof(object_green),0);
	std::cout << object_green;

	//receive blue object status: seen 0-1,x-pos 0-65535,y-pos 0-65535
	char object_blue[16];
	recv(s,&object_blue,sizeof(object_blue),0);
	std::cout << object_blue;

	//receive pink object status: seen 0-1,x-pos 0-65535,y-pos 0-65535
	char object_pink[16];
	recv(s,&object_pink,sizeof(object_pink),0);
	std::cout << object_pink;

	//receive yellow object status: seen 0-1,x-pos 0-65535,y-pos 0-65535
	char object_yellow[17];
	recv(s,&object_yellow,sizeof(object_yellow),0);
	std::cout << object_yellow << std::endl;

	close(s);
}
