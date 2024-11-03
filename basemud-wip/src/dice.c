/*
 * dice.c
 * Random dice module
*/

#include <sys/stat.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <string.h>

#include "mud.h"

/////////////////////////////////
// The random number generator //
/////////////////////////////////

// get some /urandom goodness!
unsigned long randseed(void)
{
        FILE *fp;
        unsigned long num;
        char buf[11];
        long x;

        if(!(fp = fopen("/dev/urandom","r")))
        {
//                mudlog("can't open /dev/urandom!");
                return 0;
        }
        fread(buf, 11, 1, fp);
        for(x = 0; x < 12; x++)
                num += buf[x];
        for(x = 0; x < 12; x++)
                num *= UMAX(1,buf[x]);
        num = abs(num);
        fclose(fp);
        return num;
}

static unsigned long s1=390451501, s2=613566701, s3=858993401;  // The seeds
static unsigned mask1, mask2, mask3;
static long shft1, shft2, shft3, k1=31, k2=29, k3=28;

// use either of the following two sets of parameters
static long q1=13, q2=2, q3=3, p1=12, p2=4, p3=17;
// static long q1=3, q2=2, q3=13, p1=20, p2=16, p3=7;


// INITIALIZATION STUFF
unsigned long randgen(void)
{
        static unsigned long b;

        b  = ((s1 << q1)^s1) >> shft1;
        s1 = ((s1 & mask1) << p1) ^ b;
        b  = ((s2 << q2) ^ s2) >> shft2;
        s2 = ((s2 & mask2) << p2) ^ b;
        b  = ((s3 << q3) ^ s3) >> shft3;
        s3 = ((s3 & mask3) << p3) ^ b;

        return (s1 ^ s2 ^ s3);
}


void rand_seed (unsigned long a, unsigned long b, unsigned long c)
{
        static unsigned long x = 4294967295U;

        shft1=k1-p1;
        shft2=k2-p2;
        shft3=k3-p3;
        mask1 = x << (32-k1);
        mask2 = x << (32-k2);
        mask3 = x << (32-k3);
        if (a > (unsigned int)(1<<shft1)) s1 = a;
        if (b > (unsigned int)(1<<shft2)) s2 = b;
        if (c > (unsigned int)(1<<shft3)) s3 = c;

        randgen();
}


void init_rand()
{
//      rand_seed(current_time-s1, current_time, current_time+s3);
//      rand_seed(s1, s2, s3);
        rand_seed(randseed(),randseed(),randseed());
//        mudlog("Random number generator initialized.");
}
///////////////////
// MUD FUNCTIONS //
///////////////////
long randnum(long start, long end)
{
        static long num;

        if(start<0 || end<0)
                return randneg(start,end);

        do
        {
                num = randgen() % (end+1);
        } while( num < start || num > end );

        return num;
}

// same as randnum, but allows negative numbers.
long randneg(long start, long end)
{
        long fromzero = 0;
        long neg=0;
        long test=0;

        if (start < 0 && end < 0)
                neg = 1;
        else if (start < 0)
                neg = 2;

        if (neg == 2)
        {
                fromzero = 0 - start;
                test = randnum(start+fromzero,end+fromzero);
        }
        else if (neg == 1)
                test = randnum(start*-1,end*-1);
        else
                test = randnum(start,end);

        return neg > 0 ? neg > 1 ? test - fromzero : test*-1 : test;
}

long dice(long howmany, long type)
{
        static long i, total;

        for( i = 0, total = 0; i < howmany; i++ )
                total += randnum(1,type);
        return total;
}


long percent(void)
{
        return randnum(1,100);
}

