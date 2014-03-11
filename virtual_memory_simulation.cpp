#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <queue>
#include <iomanip>
#include <algorithm>
 #include <sys/types.h>
 #include <unistd.h>

using namespace std;

struct Process
{
	int process_id;
	queue<short> req_page;			// contain the list of request
	int num_fault;
	//short address[50];
	int num_access;
	int done;			// 0: not done, 1: done
};

struct Memory
{
	int process_id;
	short page_num;
};

struct DPT
{
	int process_id;
	int page_num;
};

struct Message
{
	char message[20];
};

//------ declare global varibales---------------------
int tp = 0;			// total number of page frames
int sl =0;			// maximum segment length (# of page)
int ps = 0;			// page size (bytes)
int r = 0;			// number of page frames per a process
int min_value = 0;
int max_value = 0;
int k =0;			// number of process
int disk_size = 0;  //disk size

Process process[10];

void split_string( char src[], char dest[][20])
{
	char *prt;
	int i = 0;
	prt = strtok (src," ");
	while (prt != NULL)
	{
		strcpy(dest[i], prt);
		++i;
		prt = strtok(NULL," ");
	}
}
void load_input (Process process[])
{
	int count = 0;
	int temp[10] = {0};
	ifstream input;
	input.open("Input.txt");
	while (!input.eof())
	{
		char data[15]="";
		char subData[2][20] = {""};
		input.getline(data,15);
		if (count < 10)
		{
			if(count <7)
				temp[count] = atoi(data);
			else
			{
				split_string (data,subData);
				int i = atoi (subData[1]);
				temp[count] = i;	
			}
		}
		else
		{
			split_string (data,subData);
			int i = atoi(subData[0]) - 100;
			process[i].process_id = atoi(subData[0]);
			int j ;
			char subString[10] = "";
			strncpy(subString,subData[1],3);
			sscanf (subString,"%x",&j);		//convert a hex char into integer
			//process[i].address[process[i].num_access] = j;
			process[i].req_page.push(j);
			process[i].num_access += 1;
		}
		++count;
	}
	tp = temp[0];
	sl = temp[1];
	ps = temp[2];
	r = temp[3];
	min_value = temp[4];
	max_value = temp[5];
	k = temp[6];
	for (int j = 7; j<10;++j)
	{
		disk_size += temp[j];
	}
}

	
void main_process(int fd1[], int fd2[], ofstream& output)
{
	close (fd1[0]);			// close read end of pipe 1
	close (fd2[1]);         // close write end of pipe 2
	char buffer1[20]="";     // buffer1 for pipe 1
	char buffer2[20] ="";		// buffer for pipe 2
	int m = k;
	
	
	//------- execute for first request of each process
	for (int i = 0; i< k; ++i)
	{
		process[i].num_fault = 0;
		if (!process[i].req_page.empty())
		{
			int request = process[i].req_page.front();
			process[i].req_page.pop();
			sprintf(buffer1,"%d %d %d %d",process[i].process_id, request,-1,1);
			write (fd1[1],buffer1,20);
		}
	}

	//------ request next page after get a response ----------
	while (m >= 1)
	{
		read(fd2[0],buffer2,20);
		char subData[2][20]= {""};
		split_string(buffer2,subData);
		int id = atoi(subData[0])- 100;
		int fault = atoi(subData[1]);
		if (fault == 0)
		{
			process[id].num_fault += 1;
		}
		int request = process[id].req_page.front();
		process[id].req_page.pop();
		if (request == -1)
		{
			m -= 1;
		}
		else
		{
			sprintf(buffer1,"%d %d %d %d",process[id].process_id, request,-1,1);
			write (fd1[1],buffer1,20);
		}
	}

	int total = 0;
	for (int l = 0; l<k; ++l)
	{
		output << "Total number of page fault of process "<<l+100<<": "<<process[l].num_fault<<endl;
		total += process[l].num_fault;
	}
	output<<"Total number page fault for all "<<m<<" processes: "<<total<<endl;

	sprintf(buffer1,"%d %d %d %d", -1, -1, -1 ,3);
	write (fd1[1],buffer1,20);
}


Memory init_memory ()
{
	Memory memory;
	memory.process_id = -1;
	memory.page_num = -1;

	return memory;
}
DPT init_dpt()
{
	DPT dpt;
	dpt.process_id = -1;
	dpt.page_num = -1;

	return dpt;
}
/*
for function moving:
	type = 0: move to the last page 
	type = 1: move to the first page
*/

void moving(Memory memory[], int id, int page, int type)
{
	int j = (id -100) *r;
	Memory *current = &memory[j];
	int position = 0;
	if (type == 0)
	{
		for (int i = j; i <j+r-1;++i)
		{
			position = i + 1;
			if ( current->page_num == page)
				current++;
			if (current->page_num == -1)
			{
				position = i;
				break;
			}
			memory[i].process_id = current->process_id;
			memory[i].page_num = current->page_num;
			current ++;
		}
		memory[position].process_id = id;
		memory[position].page_num = page;
	}
	else
	{
		current = &memory[j+r-1];
		for (int i = j+r-1; i>j;--i)
		{
			if(current->page_num == page)
				current --;
			memory[i].process_id = current->process_id;
			memory[i].page_num = current->page_num;
			current --;
		}
		memory[j].process_id = id;
		memory[j].page_num = page;
	}
}

/*
for function replacement:
	type = 0: replace first page, and then move it to the last page in frame table
	type = 1: replace the first page
*/

int replacement(Memory memory[], int id, int page, int type)
{
	int j = (id -100) *r;
	int rep_page = -1;
	rep_page= memory[j].page_num;
	memory[j].page_num = page;
	memory[j].process_id = id;
	if (type == 0)
	{
		moving (memory,id,page,0);
	}
	return rep_page;
}
int FIFO (Memory memory[], int id, int page, int *rep_page)
{
	int j = (id -100) *r;
	int found = 0;
	for (int i = j; i<j+r; ++i)
	{
		if ( page == memory[i].page_num)
		{
			found = 1;
			break;
		}
		else
			found = 0;
	}

	if (found == 0)
	{
		int found1 = 0;   // check free frame 
		for (int i = j; i<j+r; ++i)
		{
			if (memory[i].page_num == -1)
			{
				found1 = 1;
				*rep_page = memory[i].page_num;
				memory[i].page_num = page;
				memory[i].process_id = id;
				break;

			}
			else
				found1 = 0;
		}
		if (found1 == 0)  // all frame are full (replace)
		{
			*rep_page = replacement(memory,id,page,0);
		}
	}

	return found;
}

int replacement_LFU (Memory memory[],int id, int page,int *frequency)
{
	int rep_page = -1;
	int j = (id -100) *r;
	int position = j;
	int temp = frequency[memory[j].page_num];
	for (int i = j+1;i <j+r;++i)
	{
		if (temp > frequency[memory[i].page_num])
		{
			temp = frequency[memory[i].page_num];
			position = i;
		}
	}
	rep_page = memory[position].page_num;
	memory[position].page_num = page;
	frequency[page] += 1;
	return rep_page;
}
int LFU (Memory memory[], int id, int page, int *rep_page, int **frequency)
{
	int j = (id -100) *r;
	int found = 0;
	for (int i = j; i<j+r; ++i)
	{
		if ( page == memory[i].page_num)
		{
			found = 1;
			frequency[id-100][page] += 1;
			break;
		}
		else
			found = 0;
	}

	if (found == 0)
	{
		int found1 = 0;   // check free frame 
		for (int i = j; i<j+r; ++i)
		{
			if (memory[i].page_num == -1)
			{
				found1 = 1;
				*rep_page = memory[i].page_num;
				memory[i].page_num = page;
				memory[i].process_id = id;
				frequency[id - 100][page] += 1;
				break;

			}
			else
				found1 = 0;
		}
		if (found1 == 0)  // all frame are full (replace)
		{
			*rep_page = replacement_LFU(memory,id,page,frequency[id-100]);
		}
	}
	return found;
}


int MRU (Memory memory[], int id, int page, int *rep_page)
{
	int j = (id -100) *r;
	int found = 0;
	for (int i = j; i<j+r; ++i)
	{
		if ( page == memory[i].page_num)
		{
			found = 1;
			moving (memory,id,page,1);
			break;
		}
		else
			found = 0;
	}

	if (found == 0)
	{
		int found1 = 0;   // check free frame 
		for (int i = j; i<j+r; ++i)
		{
			if (memory[i].page_num == -1)
			{
				found1 = 1;
				*rep_page = memory[i].page_num;
				memory[i].page_num = page;
				memory[i].process_id = id;
				moving (memory,id,page,1);
				break;

			}
			else
				found1 = 0;
		}
		if (found1 == 0)  // all frame are full (replace)
		{
			*rep_page = replacement(memory,id,page,1);
		}
	}

	return found;
}

int Random (Memory memory[], int id, int page, int *rep_page)
{
	int j = (id -100) *r;
	int found = 0;

	for (int i = j; i<j+r; ++i)
	{
		if ( page == memory[i].page_num)
		{
			found = 1;
			break;
		}
		else
			found = 0;
	}

	if (found == 0)
	{
		int found1 = 0;   // check free frame 
		for (int i = j; i<j+r; ++i)
		{
			if (memory[i].page_num == -1)
			{
				found1 = 1;
				*rep_page = memory[i].page_num;
				memory[i].page_num = page;
				memory[i].process_id = id;
				break;

			}
			else
				found1 = 0;
		}
		if (found1 == 0)  // all frame are full (replace)
		{
			int rand_num = rand() % r;
			*rep_page = memory[j+rand_num].page_num;
			memory[j+rand_num].page_num = page;
			memory[j+rand_num].process_id = id;
		}
	}


	return found;
}

void moving_working_set(int *set_page)
{
	int *current = &set_page[1];
	int temp = set_page[0];
	for (int i = 0; i< r-1;++i)
	{
		set_page[i] = *current;
		current ++;
	}
	set_page[r-1] = temp;	
}

int checking_working_set(int *set_page,int page)
{
	int found = 0;
	for (int i = 0; i< r;++i)
	{
		if (set_page[i] == -1 )
		{
			set_page[i] = page;
			found = 1;
			break;
		}
		else
			found = 0;
	}
	return found;
}

int create_working_set( Memory memory[], int id, int *set_page)
{
	int j = (id -100) *r;
	int rep_page = -1;
	int found = 0;

	if (memory[j].page_num != -1)
	{
		int temp = memory[j].page_num;
		for (int i = 0; i< r;++i)
		{
			if (temp == set_page[i])
			{
				found = 1;
				break;
			}
			else
				found = 0;
		}
		if (found == 0)
		{
			rep_page = temp;
		}
	}

	for (int i = j; i<j+r;++i)
	{
		memory[i].page_num = -1;
		memory[i].process_id = id;
	}
	int i = j;
	for (int l = 0; l< r;++l)
	{
		int temp1 = set_page[l];
		int found1 = 0;
		for (int t = j; t< j+r; ++t)
		{
			if (temp1 == memory[t].page_num)
			{
				found1 = 1;
				break;
			}
			else
				found1 = 0;
		}
		if (found1 == 0)
		{
			memory[i].page_num =temp1;
			++i;
		}
	}
	return rep_page;
}
int Working_Set (Memory memory[], int id, int page, int *rep_page, int **set_page)
{
	int j = (id -100) *r;
	int found = 0;
	for (int i = j; i<j+r; ++i)
	{
		if ( page == memory[i].page_num)
		{
			found = 1;
			break;
		}
		else
			found = 0;
	}
	int found1= 0;			// 0: full, 1: not full
	found1 = checking_working_set(set_page[id-100],page);
	if(found1 == 0)
	{
		set_page[id-100][0] = page;
		moving_working_set(set_page[id-100]);
	}
	*rep_page = create_working_set( memory, id, set_page[id-100]);
	return found;
}
void page_manager(int fd1[], int fd2[],int fd3[],int type, ofstream& output)
{
	close (fd1[1]);			// close write end of pipe 1
	close (fd2[0]);         // close end end of pipe 2
	close (fd3[0]);

	char buffer1[20] ="";
	char buffer2[20] = "";
	char buffer3[20] ="";

	Memory *memory = new Memory[tp];
	for (int i = 0; i<tp;++i)
	{
		memory[i] = init_memory ();
	}

	DPT *dpt = new DPT[disk_size];
	for (int i = 0; i< disk_size;++i)
	{
		dpt[i] = init_dpt();
	}

	int **set_page;
	set_page = new int *[k];
	for (int i = 0; i< k;++i)
	{
		set_page[i]= new int [r];
	}

	for (int i = 0; i<k;++i)
	{
		for (int j =0; j<r;++j)
		{
			set_page[i][j] = -1;
		}
	}

	int **frequency;
	frequency = new int *[k];
	for (int i = 0; i <k;++i)
	{
		frequency[i] = new int[16];
	}

	for (int i = 0; i<k;++i)
	{
		for (int j =0; j<16;++j)
		{
			frequency[i][j] = 0;
		}
	}

	while (1)
	{
		int fault = 0;				// 0 : fault, 1: not fault
		int IO = 0;			// 0: not I/O, 1: I/O
		int rep_page = -1;
		read (fd1[0], buffer1,20);
		
		char subData[3][20] ={""};
		split_string (buffer1,subData);
		int id = atoi(subData[0]);
		int page = atoi(subData[1]);
		int page1 = atoi(subData[2]);
		int value = atoi(subData[3]);		// check message from main process or disk driver
		if (value == 1)
		{
			switch (type)
			{
				case 0:
					
					fault = FIFO (memory,id,page,&rep_page);
					break;
				case 1:
					
					fault = LFU (memory,id,page,&rep_page,frequency);
					break;
				case 2:
					
					fault = MRU(memory,id,page,&rep_page);
					break;
				case 3:
					
					fault = Random(memory,id,page,&rep_page);
					break;
				case 4:
					
					fault = Working_Set(memory,id,page,&rep_page,set_page);
					break;
			}
			if (fault == 0)
			{
				if(rep_page == -1)
				{
					if (dpt[(id-100)*16 +page].page_num != -1)
						IO = 1;
				}
				else
				{
					IO = 1;
				}
				if (IO == 1)
				{
					sprintf(buffer3,"%d %d %d %d",id, page,rep_page,1);
					write(fd3[1],buffer3,20);
				}
				else
				{
					sprintf(buffer2,"%d %d",id,fault);
					write (fd2[1],buffer2,20);
				}
			}
			else
			{
				sprintf(buffer2,"%d %d",id,fault);
				write (fd2[1],buffer2,20);
			}
		}
		else
		{
			if (value == 2)
			{
				if(dpt[(id-100)*16 + page].page_num != -1)
				{
					dpt[(id-100)*16 + page].page_num = -1;
					output<<"process id: "<<id<<", swap, page No: "<<page<<endl;
				}
				if (page1 != -1)
				{
					dpt[(id-100)*16 + page1].process_id  = id;
					dpt[(id-100)*16 + page1].page_num = page1;
					output<<"process id: "<<id<<", write, page No: "<<page1<<endl;
				}
				sprintf(buffer2, "%d %d",id,0);
				write(fd2[1],buffer2,20);
			}
			else
			{
				break;
			}
		}
	}

	sprintf(buffer3,"%d %d %d %d",-1, -1, -1, 3);
	write (fd3[1],buffer3,20);
}

void disk_driver(int fd1[], int fd3[],int fd4[])
{
	close (fd3[1]);
	close (fd1[0]);
	close (fd4[0]);

	char buffer1[20]= "";
	char buffer3[20] ="";
	char buffer4[20] = "";
	int status = 0;
	queue<Message> qDisk;
	while(1)
	{
		read(fd3[0], buffer3,20);
		char subData[2][20] ={""};
		split_string (buffer3,subData);
		int id = atoi(subData[0]);
		int page = atoi(subData[1]);
		int rep_page = atoi(subData[2]);
		int value = atoi(subData[3]);			//1 : from process, 2: from disk
		if (value == 1)
		{
			Message temp;
			sprintf(temp.message,"%d %d %d",id,page, rep_page);
			qDisk.push(temp);
		}
		else
		{
			if( value == 2)
			{
				status = 0;
				sprintf(buffer1,"%d %d %d %d", id,page,rep_page,2);
				write(fd1[1],buffer1,20);
			}
			else
			{
				break;
			}
		}
		if(status == 0)
		{
			if(!qDisk.empty())
			{
				Message temp;
				temp = qDisk.front();
				qDisk.pop();

				strcpy(buffer4,temp.message);
				write(fd4[1],buffer4,20);
				status = 1;
			}
		}
	}

	sprintf(buffer4,"-1",20);
	write(fd4[1],buffer4,20);
}

void disk (int fd3[], int fd4[])
{
	close (fd3[0]);
	close (fd4[1]);

	char buffer3[20]="";
	char buffer4[20]="";

	while (1)
	{
		read(fd4[0], buffer4,20);
		int a = atoi(buffer4);
		if( a != -1)
		{
			usleep(10);
			char temp[20] ="";
			strcpy(temp,buffer4);
			sprintf(buffer3,"%s %d", temp, 2);
			write(fd3[1],buffer3,20);
		}
		else
			break;
	}
}
int main (int argc, char *argv[])
{
	load_input (process);
	

	//for (int type = 0; type <5; ++type)
	//{
	int type = atoi(argv[1]);
		int fd1[2], fd2[2],fd3[2],fd4[2];
		pipe(fd1);
		pipe(fd2);
		pipe(fd3);
		pipe(fd4);

		char buffer[2];
		sprintf(buffer,"%d",type);
		char filename[13]="";
		sprintf(filename,"output%s.txt",buffer);
		ofstream output;
		output.open (filename,ios::app);
		switch (type)
		{
			case 0:
				output<<"Algorithm: FIFO"<<endl;
				break;
			case 1:
				output<<"Algorithm: LFU"<<endl;
				break;
			case 2:
				output<<"Algorithm: MRU"<<endl;
				break;
			case 3:
				output<<"Algorithm: Random"<<endl;
				break;
			case 4:
				output<<"Algorithm: Working Set"<<endl;
				break;
		}
		if (fork() > 0)
		{
			main_process(fd1,fd2,output);
			wait();
		}
		else
		{
			if (fork() == 0)
			{
				int fd3[2], fd4[2];
				pipe(fd3);
				pipe(fd4);
				pid_t pid ;
				pid = fork();
				if (pid > 0)
				{
					page_manager(fd1,fd2,fd3,type,output);
					wait();
				}
				else
				{
					if ( pid  == 0)
					{
						pid_t pid1;
						pid1 = fork();
						if (pid1 > 0)
						{
							disk_driver(fd1,fd3,fd4);
							wait();
						}
						else
						{
							if (pid1 == 0)
							{
								disk (fd3,fd4);
								_exit(0);
							}
						}
						_exit(0);
					}
				}
				_exit(0);
			}
		}
	//}
	return 0;
}
