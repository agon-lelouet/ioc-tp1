//------------------------------------------------------------------------------
// Headers that are required for printf and mmap
//------------------------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <pthread.h>

//------------------------------------------------------------------------------
// GPIO ACCES
//------------------------------------------------------------------------------

#define BCM2835_PERIPH_BASE     0x20000000
#define BCM2835_GPIO_BASE       ( BCM2835_PERIPH_BASE + 0x200000 )

#define GPIO_LED0   4
#define GPIO_LED1   17
#define GPIO_BP     18

#define GPIO_FSEL_INPUT  0
#define GPIO_FSEL_OUTPUT 1

struct thread_args {
    uint32_t pin;
    int period;
};

int BP_ON = 0;
int BP_OFF = 0;

struct gpio_s
{
    uint32_t gpfsel[7];
    uint32_t gpset[3];
    uint32_t gpclr[3];
    uint32_t gplev[3];
    uint32_t gpeds[3];
    uint32_t gpren[3];
    uint32_t gpfen[3];
    uint32_t gphen[3];
    uint32_t gplen[3];
    uint32_t gparen[3];
    uint32_t gpafen[3];
    uint32_t gppud[1];
    uint32_t gppudclk[3];

    uint32_t test[1];
};

struct gpio_s *gpio_regs_virt; 


static void 
gpio_fsel(uint32_t pin, uint32_t fun)
{
    uint32_t reg = pin / 10;
    uint32_t bit = (pin % 10) * 3;
    uint32_t mask = 0b111 << bit;
    gpio_regs_virt->gpfsel[reg] = (gpio_regs_virt->gpfsel[reg] & ~mask) | ((fun << bit) & mask);
}

static uint32_t
gpio_read(uint32_t pin) {
    uint32_t reg = pin / 32;
    uint32_t bit = pin % 32;
    return ((gpio_regs_virt->gplev[reg]) >> bit) & 0x1;
}

static void 
gpio_write (uint32_t pin, uint32_t val)
{
    uint32_t reg = pin / 32;
    uint32_t bit = pin % 32;
    if (val == 1) 
        gpio_regs_virt->gpset[reg] = (1 << bit);
    else
        gpio_regs_virt->gpclr[reg] = (1 << bit);
}

void
delay ( unsigned int milisec )
{
    struct timespec ts, dummy;
    ts.tv_sec  = ( time_t ) milisec / 1000;
    ts.tv_nsec = ( long ) ( milisec % 1000 ) * 1000000;
    nanosleep ( &ts, &dummy );
}

void
blink( void* argsptr )
{
    // cast nécessaire
    struct thread_args* args = (struct thread_args*)argsptr;
    // Setup GPIO of pin to output
    // ---------------------------------------------
    
    gpio_fsel( args->pin, GPIO_FSEL_OUTPUT );

    // Blink led at frequency of 1Hz
    // ---------------------------------------------

    uint32_t val = 0;

    printf ( "-- info: start blinking led %d.\n", args->pin );

    while (1) {
        gpio_write ( args->pin, val );
        delay ( args->period );
        val = 1 - val;
    }

    pthread_exit(NULL);
}

void
detect_press ( void* argsptr ) {
    // polls the button regularly, (else it causes issues) and detect press
    struct thread_args* args = (struct thread_args*)argsptr;
    int oldvalue = 1;
    int curvalue = 1;
    while(1) {
        curvalue = gpio_read(GPIO_BP);
        if (oldvalue != curvalue) {
            if (curvalue == 0) {
                printf("detected press\n");
                BP_ON = 1;
            } else {
                printf("detected release\n");
                BP_OFF = 1;
            }
            printf("%d %d\n", BP_ON, BP_OFF);
            oldvalue = curvalue;
        }
        delay(args->period);
    }
}

void
toggle_led( void* argsptr ) {
    // polls the global variables corresponding to the button press, then release
    // toggles the led after a press / release
    struct thread_args* args = (struct thread_args*)argsptr;
    uint32_t val = 0;
    while (1) {
        if (BP_ON) {
            if (BP_OFF) {
                gpio_write(GPIO_LED1, val);
                val = 1 - val;
                BP_ON = 0;
                BP_OFF = 0;
            }
        }
        if (BP_OFF) {
            BP_ON = 0;
            BP_OFF = 0;
        }
        delay(args->period);
    }
}

//------------------------------------------------------------------------------
// Access to memory-mapped I/O
//------------------------------------------------------------------------------

#define RPI_PAGE_SIZE           4096
#define RPI_BLOCK_SIZE          4096

static int mmap_fd;

static int
gpio_mmap ( uint32_t volatile ** ptr )
{
    void * mmap_result;

    mmap_fd = open ( "/dev/mem", O_RDWR | O_SYNC );

    if ( mmap_fd < 0 ) {
        return -1;
    }

    mmap_result = mmap (
        NULL
      , RPI_BLOCK_SIZE
      , PROT_READ | PROT_WRITE
      , MAP_SHARED
      , mmap_fd
      , BCM2835_GPIO_BASE );

    if ( mmap_result == MAP_FAILED ) {
        close ( mmap_fd );
        return -1;
    }

    *ptr = ( uint32_t volatile * ) mmap_result;

    return 0;
}

void
gpio_munmap ( void * ptr )
{
    munmap ( ptr, RPI_BLOCK_SIZE );
}

//------------------------------------------------------------------------------
// Main Programm
//------------------------------------------------------------------------------

int
main ( int argc, char **argv )
{

    // map GPIO registers
    // ---------------------------------------------

    if ( gpio_mmap ((volatile uint32_t**)&gpio_regs_virt ) < 0 ) {
        printf ( "-- error: cannot setup mapped GPIO.\n" );
        exit ( 1 );
    }

    // Setup GPIO of LED0 to output
    // ---------------------------------------------
    
    gpio_fsel(GPIO_BP, GPIO_FSEL_INPUT);
    gpio_fsel(GPIO_LED0, GPIO_FSEL_OUTPUT);
    gpio_fsel(GPIO_LED1, GPIO_FSEL_OUTPUT);

    int period0 = 100;
    int delayval = 20;

    if (argc > 1) {
        period0 = atoi( argv[1] );
    }

    if (argc > 2) {
        delayval = atoi( argv[2] );
    }

    pthread_t threadblink;
    struct thread_args argsblink = { GPIO_LED0, period0 };
    pthread_t threaddetect;
    struct thread_args argsdetect = { 0, delayval };
    pthread_t threadtoggle;
    struct thread_args argstoggle = { 0, delayval };


    if (pthread_create(&threadblink, NULL, (void*)blink, (void*)&argsblink) != 0) {
        perror("error creating thread blink");
    }

    if (pthread_create(&threaddetect, NULL, (void*)detect_press, (void*)&argsdetect) != 0) {
        perror("error creating thread detect_press");
    }

    if (pthread_create(&threadtoggle, NULL, (void*)toggle_led, (void*)&argstoggle) != 0) {
        perror("error creating thread toggle_led");
    }

    pthread_join(threadblink, NULL);
    pthread_join(threaddetect, NULL);
    pthread_join(threadtoggle, NULL);

    return 0;
}
