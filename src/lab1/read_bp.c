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
gpio_clear_event (uint32_t pin)
{
    uint32_t reg = pin / 32;
    uint32_t bit = pin % 32;
    gpio_regs_virt->gpeds[reg] = (1 << bit);
}

static void 
gpio_enable_edge_detect (uint32_t pin, uint32_t val, int rising)
{
    uint32_t reg = pin / 32;
    uint32_t bit = pin % 32;
    if (rising) 
        gpio_regs_virt->gpren[reg] = (1 << bit);
    else
        gpio_regs_virt->gpfen[reg] = (1 << bit);
}


static uint32_t
gpio_read(uint32_t pin) {
    uint32_t reg = pin / 32;
    uint32_t bit = pin % 32;
    return ((gpio_regs_virt->gplev[reg]) >> bit) & 0x1;
}

void
delay ( unsigned int milisec )
{
    struct timespec ts, dummy;
    ts.tv_sec  = ( time_t ) milisec / 1000;
    ts.tv_nsec = ( long ) ( milisec % 1000 ) * 1000000;
    nanosleep ( &ts, &dummy );
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

    int delayval = 100;

    if (argc > 1) {
        delayval = atoi( argv[1] );
    }

    // 
    // ---------------------------------------------
    int oldvalue = 1;
    int curvalue = 1;
    int togglevalue = 0;
    while(1) {
        oldvalue = curvalue;
        curvalue = gpio_read(GPIO_BP);
        if (oldvalue != curvalue) {
            printf("switch\n");
            togglevalue = 1- togglevalue;
            printf("%d\n", togglevalue);
        }

        
        delay(delayval);
    }

    printf("%d\n", togglevalue);

    return 0;
}
