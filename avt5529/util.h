#ifndef UTIL_H
#define UTIL_H

    void int2asc(unsigned int liczba, unsigned char* ascii)
    {
        unsigned char
            i,
            temp;

        for (i = 0; i < 4; i++)
        {
            temp = liczba % 10;
            liczba /= 10;
            ascii[i] = '0' + temp;
        }
    }


    //void fixedPoint2Ascii(unsigned int liczba, unsigned char* ascii)

#endif