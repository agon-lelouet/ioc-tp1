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

struct fsel_s {
    uint32_t s0:3;
    uint32_t s1:3;
    uint32_t s2:3;
    uint32_t s3:3;
}; // cette syntaxe du C permet de faire un masque automatique

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

    // Setup GPIO of led to output
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

int
main ( int argc, char **argv )
{
    // Get args
    // ---------------------------------------------

    int period0 = 1000; /* default = 1Hz */

    if ( argc > 1 ) {
        period0 = atoi ( argv[1] );
    }

    uint32_t volatile * gpio_base = 0;

    // map GPIO registers
    // ---------------------------------------------

    if ( gpio_mmap ((volatile uint32_t**)&gpio_regs_virt ) < 0 ) {
        printf ( "-- error: cannot setup mapped GPIO.\n" );
        exit ( 1 );
    }

    gpio_fsel(GPIO_LED0, GPIO_FSEL_OUTPUT);
    pthread_t thread0;

    // we use a struct to pass args to the thread we create
    struct thread_args args0 = { GPIO_LED0, period0 };

    // same as blink0_pt.c, but using pthread
    if (pthread_create(&thread0, NULL, (void*)blink, (void*)&args0) != 0) {
        perror("error creating thread 0");
    }

    pthread_join(thread0, NULL);

    return 0;
}
