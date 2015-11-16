#include <stddef.h>
#include <stdint.h>
#include "reg.h"
#include "threads.h"

/* USART TXE Flag
 * This flag is cleared when data is written to USARTx_DR and
 * set when that data is transferred to the TDR
 */
#define USART_FLAG_TXE	((uint16_t) 0x0080)
#define USART_FLAG_RXNE ((uint16_t) 0x0020)
#define MAX_COMMAND_LENGTH (50)
#define MAX_ARGC (5)

void usart_init(void)
{
	*(RCC_APB2ENR) |= (uint32_t) (0x00000001 | 0x00000004);
	*(RCC_APB1ENR) |= (uint32_t) (0x00020000);

	/* USART2 Configuration, Rx->PA3, Tx->PA2 */
	*(GPIOA_CRL) = 0x00004B00;
	*(GPIOA_CRH) = 0x44444444;
	*(GPIOA_ODR) = 0x00000000;
	*(GPIOA_BSRR) = 0x00000000;
	*(GPIOA_BRR) = 0x00000000;

	*(USART2_CR1) = 0x0000000C;
	*(USART2_CR2) = 0x00000000;
	*(USART2_CR3) = 0x00000000;
	*(USART2_CR1) |= 0x2000;
}

void print_str(const char *str)
{
	while (*str) {
		while (!(*(USART2_SR) & USART_FLAG_TXE));
		*(USART2_DR) = (*str & 0xFF);
		str++;
	}
}

char usart2_rx()
{
	while(1)
	{
		if((*USART2_SR) & (USART_FLAG_RXNE))
			return (*USART2_DR) & 0xff;
	}
}

int strcmp(char *str1, char *str2){
	int index = 0;
	while(str1[index] != '\0'){
		if(str1[index] != str2[index])
			return 0;
		index++;
	}
	if(str2[index+1] != '\0')
		return 0;
	else
		return 1;
}

void int2char(int x, char *str){
	int num;
	int tmp = x;
	for(int i = 0; i < 10; i++){
		num = tmp % 10;
		str[10-i-1] = '0' + num;
		tmp = tmp / 10;
	}
}

int fibonacci(int x){
	if(x==0) return 0;
	if(x==1 || x==2) return 1;
	return fibonacci(x-1)+fibonacci(x-2);
}
extern int fibonacci(int x);

void shell(void *userdata)
{
	char ichar;
	char command_buffer[MAX_COMMAND_LENGTH+1];
	char command[MAX_ARGC][MAX_COMMAND_LENGTH+1];
	int argv_index = 0;
	int char_index = 0;
	int index = 0;
	int fib_result = 0;
	char fib_result_char[10];
	while(1){
		while(char_index<=MAX_COMMAND_LENGTH){
			char_index++;
			command_buffer[char_index] = '\0';
		}
		print_str("arno@mini-arm-os:~$ ");
		char_index = 0;
		
		//command input
		while(1){
			ichar = usart2_rx();
			if(ichar == 13){//enter
				print_str("\n");
				break;
			}
			else if(ichar == 127){//backspace
				if(char_index!=0){
					print_str("\b");
					print_str(" ");
					print_str("\b");
					char_index--;
					command_buffer[char_index] = '\0';
				}
			}
			else{
				if(char_index<MAX_COMMAND_LENGTH){
					command_buffer[char_index] = ichar;
					print_str(&command_buffer[char_index]);
					char_index++;
				}
			}
		}
		
		//command identification
		if(char_index != 0){
			for(argv_index = 0; argv_index<MAX_ARGC; argv_index++)
				for(char_index = 0; char_index<=MAX_COMMAND_LENGTH; char_index++)
					command[argv_index][char_index] = '\0';
			argv_index = 0;
			char_index = 0;
			for(index = 0; index<MAX_COMMAND_LENGTH; index++){
				if(command_buffer[index] == ' '){
					if(char_index!=0){
						char_index = 0;
						argv_index++;
					}
				}
				else if(command_buffer[index] != '\0'){
					command[argv_index][char_index] = command_buffer[index];
					char_index++;
				}
				else {}
			}
			
			if(strcmp(command[0], "fibonacci\0")){
				fib_result = fibonacci(5);
				int2char(fib_result, fib_result_char);
				print_str(fib_result_char);
				print_str("\n");
			}
			
		}
		char_index = 0;
		argv_index = 0;
		index = 0;
	}
}

/* 72MHz */
#define CPU_CLOCK_HZ 72000000

/* 100 ms per tick. */
#define TICK_RATE_HZ 10

int main(void)
{
	const char *str1 = "shell";

	usart_init();

	if (thread_create(shell, (void *) str1) == -1)
		print_str("Shell creation failed\r\n");

	/* SysTick configuration */
	*SYSTICK_LOAD = (CPU_CLOCK_HZ / TICK_RATE_HZ) - 1UL;
	*SYSTICK_VAL = 0;
	*SYSTICK_CTRL = 0x07;

	thread_start();

	return 0;
}
