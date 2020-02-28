#include <stdlib.h>  //pre použitie malloc() a free()
#include <stdio.h>   //pre stdout, ktorý bol použitý v get_pressed_key()
#include <stdbool.h> //pre používanie dátového typu "bool"
#ifdef __unix__
#include <unistd.h>  //pre použitie sleep()
#include <termios.h> //nejaké veci pre funkciu get_pressed_key()
//#include <threads.h> C11 obsahuje podporu pre vlákna, ale podpora kompilérov pre tento header je malá.
#include <pthread.h> //namiesto threads.h bol použitý pthread.h
#endif

#ifdef _WIN32

#include <windows.h>
#include <conio.h>
#endif


#ifdef __unix__
char _getch(void)
{
	// getch získa klávesu stlačenú v reálnom čase. Je dôležitá pre pohyb hada
	char buf = 0;
	struct termios old = { 0 };
	fflush(stdout);
	if (tcgetattr(0, &old) < 0)
	{
		perror("tcsetattr()");
	}
	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &old) < 0)
	{
		perror("tcsetattr ICANON");
	}
	if (read(0, &buf, 1) < 0)
	{
		perror("read()");
	}
	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;
	if (tcsetattr(0, TCSADRAIN, &old) < 0)
	{
		perror("tcsetattr ~ICANON");
	}
	printf("%c\n", buf);
	return buf;
}
#endif
struct arena_t
{
	int width;
	int height;
	int* coords;
};
/*časti hada sú v súradniciach uložené ako záporné hodnoty.
časti hada sú tiež očíslované od -1 nižšie. -1 je hodnota miesta kde sa začína had
bezproblémový pohyb hada, tak aby hra vedela kde sa had posunul, som vedel vyriešiť len cez označenie poradia
jednolivých častí hada
*/

struct snake_t
{
	bool fruit_eaten;
	bool is_alive;
	int length;
	int direction; //1 right, 2 left, 3 up, 4 down
	bool can_change_direction;
};


//hlavička funkcie handle_user_input je trochu iná pre windows a linux
#ifdef __unix__ //linux
	void* handle_user_input(void* param)
#endif
#ifdef _WIN32 //windows
	DWORD WINAPI handle_user_input(LPVOID param)
#endif
{
	//1 right, 2 left, 3 up, 4 down

	struct snake_t* snake = (struct snake_t*)param;

	while (snake->is_alive)
	{
		if (snake->can_change_direction) {
			int pressed_key = _getch();

			int key_w = 119;
			int key_s = 115;
			int key_a = 97;
			int key_d = 100;

			int key_up = 65;
			int key_down = 66;
			int key_left = 68;
			int key_right = 67;

			if (pressed_key == key_right || pressed_key == key_d)
			{
				if (snake->direction != 1 && snake->direction != 2)
				{
					snake->direction = 1;
					snake->can_change_direction = false;
				}
			}
			else if (pressed_key == key_left || pressed_key == key_a)
			{
				if (snake->direction != 1 && snake->direction != 2)
				{
					snake->direction = 2;
					snake->can_change_direction = false;

				}
			}
			else if (pressed_key == key_up || pressed_key == key_w)
			{
				if (snake->direction != 3 && snake->direction != 4)
				{
					snake->direction = 3;
					snake->can_change_direction = false;

				}
			}
			else if (pressed_key == key_down || pressed_key == key_s)
			{
				if (snake->direction != 3 && snake->direction != 4)
				{
					snake->direction = 4;
					snake->can_change_direction = false;
				}
			}
		}
	}
	exit(0);
}

void game_loop(struct arena_t* arena, struct snake_t* snake);

//funkcia pre vytvorenie objektov v hre
bool init_game_objects(struct arena_t* arena, struct snake_t* snake);

int main(void)
{
	struct arena_t arena;
	struct snake_t snake;

	bool status = init_game_objects(&arena, &snake);
	if (status)
	{
		printf("game init");

#ifdef __unix__
		pthread_t thread1;

		int status = pthread_create(&thread1, NULL, handle_user_input, (void*)snake);

		if (status == 0)
		{ //pthread_create navráti 0 ak úspešne vytvorí vlákno
			game_loop(&arena, &snake);
		}
		else
		{
			printf("init failed");
		}
	}
#endif
#ifdef _WIN32
	HANDLE thread_handle = CreateThread(0, 0, handle_user_input, (LPVOID)&snake, 0, 0);
	printf("game loop");
	game_loop(&arena, &snake);
#endif


	return 0;
	}
}
void game_loop(struct arena_t* arena, struct snake_t* snake)
{
	while (snake->is_alive)
	{
		int count = 0;

		for (int y = 0; y < arena->height; y++)
		{
			for (int x = 0; x < arena->width; x++)
			{

				int value_at_coord = *(int*)(arena->coords + count);

				if (value_at_coord >= 0)
				{
					if (value_at_coord == 0)
						printf(" ");

					if (value_at_coord == 2)
						printf("#");

					if (value_at_coord == 3)
						printf("A");
				}
				else
				{
					if (value_at_coord == -1)
					{
						printf("O");
					}
					else
					{
						printf("o");
					}
				}

				count++;
			}
			printf("\n");
		}

		//pohyb hada

		//pomocné pole, kde budú uložené nové súradnice hada ktoré sa majú vykresliť
		//nakoniec som sa rozhodol vyhradiť dynamicky miesto pre súradnice hada, aby som to miesto na konci mohol uvolnit.
		int* new_snake_coords = (int*)malloc(sizeof(int) * snake->length * 2);

		//tieto 3 cykly vnorené jeden v druhom prejdu cez všetky súradnice v hre, nájdu súradnice kde je uložený had
		//vypočítajú kde budú súradnice hada keď sa had posunie, a nové súradnice uložia do new_snake_coords
		//potom sa z informácií uložených v new_snake_coords vykreslí nový had

		for (int z = 1; z <= snake->length; z++)
		{
			int count = 0;

			for (int y = 0; y < arena->height; y++)
			{
				for (int x = 0; x < arena->width; x++)
				{
					int value_at_coordinate = *(int*)(arena->coords + count);
					//získa sa hlava hada, miesto kde had začína. Tá má vždy hodnotu 1
					if (value_at_coordinate == -(z))
					{

						//ak sa blok rovná hľadanej časti hada
						if (value_at_coordinate == -1)
						{
							if (snake->direction == 1)
							{
								*(new_snake_coords + ((z - 1) * 2)) = x + 1;
								*(new_snake_coords + ((z - 1) * 2) + 1) = y;

								int value_at_coordinate1 = *(arena->coords + (y * arena->width + x + 1));

								if (value_at_coordinate1 == 2 || value_at_coordinate1 < 0)
								{
									snake->is_alive = false;
								}

								if (value_at_coordinate1 == 3)
								{
									snake->fruit_eaten = true;
								}
							}

							if (snake->direction == 2)
							{
								*(new_snake_coords + (z - 1)) = x - 1;
								*(new_snake_coords + (z - 1) + 1) = y;

								int value_at_coordinate1 = *(arena->coords + (y * arena->width + x - 1));

								if (value_at_coordinate1 == 2 || value_at_coordinate1 < 0)
								{
									snake->is_alive = false;
								}

								if (value_at_coordinate1 == 3)
								{
									snake->fruit_eaten = true;
								}
							}

							if (snake->direction == 3)
							{
								*(new_snake_coords + (z - 1)) = x;
								*(new_snake_coords + (z - 1) + 1) = y - 1;

								int value_at_coordinate1 = *(arena->coords + ((y - 1) * arena->width + x));

								if (value_at_coordinate1 == 2 || value_at_coordinate1 < 0)
								{
									snake->is_alive = false;
								}

								if (value_at_coordinate1 == 3)
								{
									snake->fruit_eaten = true;
								}
							}

							if (snake->direction == 4)
							{
								*(new_snake_coords + (z - 1)) = x;
								*(new_snake_coords + (z - 1) + 1) = y + 1;

								int value_at_coordinate1 = *(arena->coords + ((y + 1) * arena->width + x));

								if (value_at_coordinate1 == 2 || value_at_coordinate1 < 0)
								{
									snake->is_alive = false;
								}

								if (value_at_coordinate1 == 3)
								{
									snake->fruit_eaten = true;
								}
							}
						}
						else
						{
							int snake_part_id_to_find = -(z - 1);
							int count1 = 0;
							for (int y1 = 0; y1 < arena->height; y1++)
							{
								for (int x1 = 0; x1 < arena->width; x1++)
								{
									int value_at_coordinate2 = *(int*)(arena->coords + count1);

									if (value_at_coordinate2 == snake_part_id_to_find)
									{
										*(new_snake_coords + ((z - 1) * 2)) = x1;
										*(new_snake_coords + ((z - 1) * 2) + 1) = y1;
									}
									count1++;
								}
							}
						}
					}
					count++;
				}
			}
		}

		//tento cyklus má za úlohu zmazaŤ stáré súradnice hada z arény.
		//nižšie v kóde sa do pamate následne zapíšu aktualizované súradnice hada, a had sa vykreslí do príkazového riadku
		count = 0;

		for (int y = 0; y < arena->height; y++)
		{
			for (int x = 0; x < arena->width; x++)
			{

				int value_at_coordinates = *(int*)(arena->coords + count);

				if (value_at_coordinates < 0)
				{
					*(int*)(arena->coords + count) = 0;
				}
				count++;
			}
		}

		//do arena->coords uložia nové súradnice, ktoré sú uložené v poli new_snake_coords
		for (int x = 0; x < snake->length; x++)
		{
			//tu ide o to, nájsť nastaviť správnu hodnotu v správnom mieste, niekde v bloku pamate (blokom pamete je v tomto prípade arena->coords)
			//na tomto mieste mi to na windowse padá
			*(arena->coords + (*(new_snake_coords + ((2 * x) + 1)) * arena->width + *(new_snake_coords + 2 * x))) = -(x + 1);
		}

		if (snake->fruit_eaten == true)
		{
			//snake->length += 1;
			snake->fruit_eaten = false; //nastavý sa aby další cyklus pracoval s tým že ovocie sa nezjedlo a had nezvečšoval naďalej svoj dĺžku
			//had zjedol ovocie, takže sa v aréne vytvorí dalšie

			int fruit_coord = 0;
			bool fruit_created = false;
			while (!fruit_created)
			{
				fruit_coord = rand();

				while (fruit_coord > arena->width* arena->height)
				{
					//náhodné číslo ktoré navráti rand() je normálne príliš veľké a je potrebné ho zmenšovať
					//kým nebude spadať do požadovanej škály povolených hodnôt
					//takže číslo sa bude deliť tromi pokiaľ nebude mať veľkosť ktorá nebude presahovať
					//maximálny počet súradníc v aréne, maximálny počet 2d blokov s ktorych je aréna zložená
					fruit_coord = fruit_coord / 3;
				}

				//tu prebehne kontrola či miesto kde sa umiestni ovocie nieje obsadené hranicou arény
				//alebo časťou hada
				int value_at_coord = *(arena->coords + fruit_coord);
				if (value_at_coord == 2 || value_at_coord < 0)
				{
					fruit_created = false;
				}
				else
				{
					fruit_created = true;
				}
			}
			//ovocie bolo vytvorené, takže sa umiestni do arény
			*(arena->coords + fruit_coord) = 3;
		}
		//
#ifdef __unix__
		usleep(300000);
		system("clear");
#endif

#ifdef _WIN32
		Sleep(300);
		system("cls");
#endif

		//po vykreslení sa had znova nastaví na to, aby mohol meniť smer keď hráč slačí šípky
		snake->can_change_direction = true;
	}
	printf("game over");
}

bool init_game_objects(struct arena_t* arena, struct snake_t* snake)
{

	if (snake != 0 && arena != 0)
	{

		snake->is_alive = true;
		snake->length = 7;
		snake->direction = 1;
		snake->fruit_eaten = false;
		snake->can_change_direction = true;

		arena->width = 60;
		arena->height = 20;
		arena->coords = (int*)malloc(arena->width * arena->height * sizeof(int));

		//tu sa musia nastaviť hodnoty počiatočných súradnicv hre
		//hodnotami počiatočných súradníc myslím také veci ako hranice arény
		int count = 0;

		for (int y = 0; y < arena->height; y++)
		{
			for (int x = 0; x < arena->width; x++)
			{
				if (x == 0 || x == arena->width - 1 || y == 0 || y == arena->height - 1)
				{
					*(arena->coords + count) = 2; //v skutonosti count * sizeof(int)
				}
				else
				{
					*(arena->coords + count) = 0;
				}
				count++;
			}
		}

		//tu sa vytvorí had
		count = 0;
		for (int x = 0; x < snake->length; x++)
		{
			*(arena->coords + (4 * arena->width + 4 + 7 - count)) = (-1 - count);
			count++;
		}

		//tu sa vytvorí ovocie. Bude vždy na tom istom mieste, ale na začiatok hry to nevadí
		*(arena->coords + 200) = 3;

		return true;
	}
	else
	{
		return false;
	}
}