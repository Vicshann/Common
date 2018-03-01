//---------------------------------------------------------------------------


/**\addtogroup AES Advanced Encryption Standard******************************
 *
 *  Advanced Encryption Standard (Rijndael) algorithm implementation.
 *
 *  @author Serge Trusov
 *  mailto: serge.trusov@gmail.com
 *
 *  @version    0.5.05.21
 *  @date       21 May 2005
 *
 *  @note   Для тестирования работы скомпилируйте файл aes.cpp
 *          определив макрос AES_TEST   @see main()
 *
 *//**@{*/ /**  @file
 *                                                                     \brief
 *  Реализация класса #aes. 
 *  Обеспечивает (де)шифрацию блока данных по алгоритму Rijndael.
 *  Реализованы варианты: AES-128, AES-192 и AES-256.            @copydoc AES
 *
 ****************************************************************************
 */ 


#include <stdlib.h>
#include "aes.h"


//=========================================================================//
//
//  Перед использованием необходимо сгенерировать 
//  расширенный ключ (из 128-ми, 192-ч или 256-ти битного)
//
CAES::status CAES::expand_key( const byte * key, unsigned int size )
{
    unsigned int i;

	size *= 8;
    //  инициализированный s_box содержит здесь ненулевое значение
    if( ! s_box[0] ) init_tables();

    //  Копируем ключ.

    u32   * ek = encryption_round_key;
    for( i = 0; i < size / (8*sizeof(u32)); i++ )
        ek[i] = get( &key[i * 4] );

    //  Создаём расширенный ключ для засшифровки

    u32 round_constant = 1;
    switch( size )
    {
    default: 
        return  wrong_key_lenght;

    #ifdef AES_128
    case 128:
        for( i = 0; i < 10; i++, ek += 4 )
        {
            ek[4] = ek[0]
                  ^ s_box[ ek[3] >>  0 & 0xFF ] << 24
                  ^ s_box[ ek[3] >> 24        ] << 16
                  ^ s_box[ ek[3] >> 16 & 0xFF ] <<  8
                  ^ s_box[ ek[3] >>  8 & 0xFF ] <<  0
                  ^ round_constant;
            ek[5] = ek[1] ^ ek[4];
            ek[6] = ek[2] ^ ek[5];
            ek[7] = ek[3] ^ ek[6];
            round_constant = lfsr2((byte)round_constant);
        }
        rounds = 10;
        break;
    #endif

    #ifdef AES_192
    case 192:
        for( i = 0; i < 8; i++, ek += 6 )
        {
            ek[6]  = ek[0]
                   ^ s_box[ ek[5] >>  0 & 0xFF ] << 24
                   ^ s_box[ ek[5] >> 24        ] << 16
                   ^ s_box[ ek[5] >> 16 & 0xFF ] <<  8
                   ^ s_box[ ek[5] >>  8 & 0xFF ] <<  0
                   ^ round_constant;
            ek[7]  = ek[1] ^ ek[6]; 
            ek[8]  = ek[2] ^ ek[7];
            ek[9]  = ek[3] ^ ek[8]; 
            ek[10] = ek[4] ^ ek[9];
            ek[11] = ek[5] ^ ek[10];
            //  упростим lfsr2, т.к кол-во итераций цикла < 9
            round_constant <<=1;// lfsr2( round_constant );
        }
        rounds = 12;
        break;
    #endif

    #ifdef AES_256
    case 256:
        for( i = 0; i < 7; i++, ek += 8 )
        {
            ek[8]  = ek[0]
                   ^ s_box[ ek[7] >>  0 & 0xFF ] << 24
                   ^ s_box[ ek[7] >> 24        ] << 16
                   ^ s_box[ ek[7] >> 16 & 0xFF ] <<  8
                   ^ s_box[ ek[7] >>  8 & 0xFF ] <<  0
                   ^ round_constant;
            ek[9]  = ek[1] ^ ek[8];
            ek[10] = ek[2] ^ ek[9];
            ek[11] = ek[3] ^ ek[10];
            ek[12] = ek[4]
                   ^ s_box[ ek[11] >> 24        ] << 24
                   ^ s_box[ ek[11] >> 16 & 0xFF ] << 16
                   ^ s_box[ ek[11] >>  8 & 0xFF ] <<  8
                   ^ s_box[ ek[11] >>  0 & 0xFF ] <<  0;
            ek[13] = ek[5] ^ ek[12];
            ek[14] = ek[6] ^ ek[13];
            ek[15] = ek[7] ^ ek[14];
            //  упростим lfsr2, т.к кол-во итераций цикла < 9
            round_constant <<=1;// lfsr2( round_constant );
        }
        rounds = 14;
        break;
    #endif
    }

    //  Создаём расширенный ключ для расшифровки

    u32   * dk = decryption_round_key;
    dk[0] = ek[0];  dk[1] = ek[1];  dk[2] = ek[2];  dk[3] = ek[3];

    for( i = 4 * (rounds-1); i; --i, dk++ )
    {
        //  Исходные данные расположены в таком порядке:
        //  ..., 9, 10, 11, 12, 5, 6, 7, 8, 1, 2, 3, 4.
        //  первый раз из 4х двигаем указатель "назад" на 7
        //  остальные три - "вперёд" на 1
        ek += i & 0x3 ? 1 : -7;
        dk[4] = rt[0][ s_box[ ek[4-1] >>  0 & 0xFF ] ]
              ^ rt[1][ s_box[ ek[4-1] >>  8 & 0xFF ] ]
              ^ rt[2][ s_box[ ek[4-1] >> 16 & 0xFF ] ]
              ^ rt[3][ s_box[ ek[4-1] >> 24        ] ];
    }
    dk[4] = ek[4-8];    dk[5] = ek[5-8];
    dk[6] = ek[6-8];    dk[7] = ek[7-8];

    return Ok;
}


//=========================================================================//
//
//  Шифрование
// 
void CAES::encrypt( const byte in[block_size], byte out[block_size] )
{
    //  Указатель на раундовые константы
    u32   * rk = encryption_round_key;

    //  Считываем блок и добавляем раундовый ключ.
    u32     a0 = get( &in[ 0] ) ^ rk[0];
    u32     a1 = get( &in[ 4] ) ^ rk[1];
    u32     a2 = get( &in[ 8] ) ^ rk[2];
    u32     a3 = get( &in[12] ) ^ rk[3];
    u32     b0, b1, b2, b3;

    //  Раунды криптования (цикл развёрнут 2x)
    for( int i = rounds; ; rk += 8 )   
    {
        b0 = fb(a0, 0) ^ fb(a1, 1) ^ fb(a2, 2) ^ fb(a3, 3) ^ rk[4];
        b1 = fb(a1, 0) ^ fb(a2, 1) ^ fb(a3, 2) ^ fb(a0, 3) ^ rk[5];
        b2 = fb(a2, 0) ^ fb(a3, 1) ^ fb(a0, 2) ^ fb(a1, 3) ^ rk[6];
        b3 = fb(a3, 0) ^ fb(a0, 1) ^ fb(a1, 2) ^ fb(a2, 3) ^ rk[7];
        
        //  Сверхумный MSVC разворачивает цикл, что раздувает размер
        //  и снижает (!) скорость на 10%.
        //  приходится добавлять лишний байт :-(.
        #if defined(_MSC_VER) && ! defined(__cplusplus_cli)
            __asm nop
        #endif

        if( ! (i -= 2) ) break;

        a0 = fb(b0, 0) ^ fb(b1, 1) ^ fb(b2, 2) ^ fb(b3, 3) ^ rk[8];
        a1 = fb(b1, 0) ^ fb(b2, 1) ^ fb(b3, 2) ^ fb(b0, 3) ^ rk[9];
        a2 = fb(b2, 0) ^ fb(b3, 1) ^ fb(b0, 2) ^ fb(b1, 3) ^ rk[10];
        a3 = fb(b3, 0) ^ fb(b0, 1) ^ fb(b1, 2) ^ fb(b2, 3) ^ rk[11];
    }
      
    //  Последний раунд (без перемешивания столбцов)
    put(
        rk[8]  ^ s_box[ b3 >> 24        ] << 24
               ^ s_box[ b2 >> 16 & 0xFF ] << 16
               ^ s_box[ b1 >>  8 & 0xFF ] <<  8
               ^ s_box[ b0 >>  0 & 0xFF ] <<  0
        , &out[ 0]
        );
    put( 
        rk[9]  ^ s_box[ b0 >> 24        ] << 24
               ^ s_box[ b3 >> 16 & 0xFF ] << 16 
               ^ s_box[ b2 >>  8 & 0xFF ] <<  8 
               ^ s_box[ b1 >>  0 & 0xFF ] <<  0
        , &out[ 4]
        );   
    put( 
        rk[10] ^ s_box[ b1 >> 24        ] << 24
               ^ s_box[ b0 >> 16 & 0xFF ] << 16
               ^ s_box[ b3 >>  8 & 0xFF ] <<  8
               ^ s_box[ b2 >>  0 & 0xFF ] <<  0
        , &out[ 8]
        );  
    put( 
        rk[11] ^ s_box[ b2 >> 24        ] << 24
               ^ s_box[ b1 >> 16 & 0xFF ] << 16
               ^ s_box[ b0 >>  8 & 0xFF ] <<  8
               ^ s_box[ b3 >>  0 & 0xFF ] <<  0
        , &out[12]
        );
}


//=========================================================================//
//
//  Расшифровывание.
// 
void CAES::decrypt( const byte in[block_size], byte out[block_size] )
{
    //  Указатель на раундовые константы
    u32   * rk = decryption_round_key;

    //  Считываем блок и добавляем раундовый ключ.
    u32     a0 = get( &in[ 0] ) ^ rk[0];
    u32     a1 = get( &in[ 4] ) ^ rk[1];
    u32     a2 = get( &in[ 8] ) ^ rk[2];
    u32     a3 = get( &in[12] ) ^ rk[3];
    u32     b0, b1, b2, b3;

    //  Раунды разкриптования (цикл развёрнут 2x)
    for( int i = rounds; ; rk += 8 )
    {
        b0 = rb(a0, 0) ^ rb(a3, 1) ^ rb(a2, 2) ^ rb(a1, 3) ^ rk[4];
        b1 = rb(a1, 0) ^ rb(a0, 1) ^ rb(a3, 2) ^ rb(a2, 3) ^ rk[5];
        b2 = rb(a2, 0) ^ rb(a1, 1) ^ rb(a0, 2) ^ rb(a3, 3) ^ rk[6];
        b3 = rb(a3, 0) ^ rb(a2, 1) ^ rb(a1, 2) ^ rb(a0, 3) ^ rk[7];

        //  Сверхумный MSVC разворачивает цикл, что раздувает размер
        //  и снижает (!) скорость.
        //  приходится добавлять лишний байт :-(.
        #if defined(_MSC_VER) && ! defined(__cplusplus_cli)
            __asm nop
        #endif

        if( ! (i -= 2) ) break;

        a0 = rb(b0, 0) ^ rb(b3, 1) ^ rb(b2, 2) ^ rb(b1, 3) ^ rk[8];
        a1 = rb(b1, 0) ^ rb(b0, 1) ^ rb(b3, 2) ^ rb(b2, 3) ^ rk[9];
        a2 = rb(b2, 0) ^ rb(b1, 1) ^ rb(b0, 2) ^ rb(b3, 3) ^ rk[10];
        a3 = rb(b3, 0) ^ rb(b2, 1) ^ rb(b1, 2) ^ rb(b0, 3) ^ rk[11];
    }
    
    //  Последний раунд (без перемешивания столбцов)
    put(
        rk[8]  ^ r_box[ b1 >> 24        ] << 24
               ^ r_box[ b2 >> 16 & 0xFF ] << 16
               ^ r_box[ b3 >>  8 & 0xFF ] <<  8
               ^ r_box[ b0 >>  0 & 0xFF ] <<  0
        , &out[ 0]
        );
    put(
        rk[9]  ^ r_box[ b2 >> 24        ] << 24
               ^ r_box[ b3 >> 16 & 0xFF ] << 16
               ^ r_box[ b0 >>  8 & 0xFF ] <<  8
               ^ r_box[ b1 >>  0 & 0xFF ] <<  0
        , &out[ 4]
        );
    put(
        rk[10] ^ r_box[ b3 >> 24        ] << 24
               ^ r_box[ b0 >> 16 & 0xFF ] << 16
               ^ r_box[ b1 >>  8 & 0xFF ] <<  8
               ^ r_box[ b2 >>  0 & 0xFF ] <<  0
        , &out[ 8]
        );
    put(
        rk[11] ^ r_box[ b0 >> 24        ] << 24
               ^ r_box[ b1 >> 16 & 0xFF ] << 16
               ^ r_box[ b2 >>  8 & 0xFF ] <<  8
               ^ r_box[ b3 >>  0 & 0xFF ] <<  0
        , &out[12]
        );
}

//@}    protected:


byte inline CAES::lfsr2( byte a )
{
    return a & 0x80 ? a<<1 ^ (byte)basis    // вычитание по модулю 2
                    : a<<1;                 // нет обратноё связи
}


//=========================================================================//
/// !.
/// Создаёт прямую и обратную таблицы замен (S-box)
/// и расширенные таблицы замен/микширования -
/// используются для оптимизации криптораундов по скорости.
///
void CAES::init_tables()
{
    //  Таблицы для быстрого умножения в поле GF(2**8)  
    byte    pow[1<<8],  log[1<<8];

    //  Заполняем их.
    unsigned int    i = 0;      //  Начальная степень omega;
    byte        a = 1;      //  omega**i в полиномиальной форме;
    log[0] = 0;             //  Ой элемент нужен для генерации S-box
    do
    {
        pow[i] = a;         //  omega**i = x7**7 + .. + x0**0;
        log[a] = (byte)i;
        //  следующий элемент таблицы выбирается 
        //  на основе полинома phi(x+1)
        a ^= lfsr2(a);      //  a *= (x + 1)
    }
    while( (byte)++i );     //  2**8 итераций

    //  Что бы не инициализировать отдельно элементы [0x63, 0]
    //  используем "лишнюю" итерацию цикла;
    //   для этого модифицируем неиспользуемый элемент таблицы.
    pow[ (1<<8) - 1 ] = 0;

    //  Генерируем прямой и обратный S-box'ы
    i = 0;
    do
    {
        byte    a = pow[(1<<8)-1 - log[i]];

        //  Умножаем на     x**4 + x**3 + x**2 + x + 1
        a ^= a << 1 ^ a << 2 ^ a << 3 ^ a << 4  // a<<1|a>>7 == a<<1^a>>7 !
          ^  a >> 4 ^ a >> 5 ^ a >> 6 ^ a >> 7  // переставим слагаемые 
                                                // что бы помочь компилятору
        //  и прибавляем    x**6 + x**5 + x**1 + x**0
          ^                (1<<6 ^ 1<<5 ^ 1<<1 ^ 1<<0);   
        
        s_box[i] = a;
        r_box[a] = i;
    }
    while( (byte)++i );     //  2**8 итераций

    //  Генерируем расширенные таблицы замен
    for( i = 0; i < 256; i++ )
    {
        byte f = (byte)s_box[i]; 
        byte f2 = lfsr2(f);
        ft[0][i] = (f ^ f2) << 24 | f << 16 | f << 8 | f2;
        ft[1][i] = rot_left_8( ft[0][i] );
        ft[2][i] = rot_left_8( ft[1][i] );
        ft[3][i] = rot_left_8( ft[2][i] );

        byte r = (byte)r_box[i];
        //  Умножение выполняется по формуле:
        //  pow[(log[x] + log[y]) % 255]    ( при x и y != 0 )
        rt[0][i] = ! r ? r :
                 (u32) pow[(0x68 + log[r]) % 255] << 24 // log[0x0B] == 0x68
               ^ (u32) pow[(0xEE + log[r]) % 255] << 16 // log[0x0D] == 0xEE
               ^ (u32) pow[(0xC7 + log[r]) % 255] <<  8 // log[0x09] == 0xC7
               ^ (u32) pow[(0xDF + log[r]) % 255] <<  0;// log[0x0E] == 0xDF
        rt[1][i] = rot_left_8( rt[0][i] );
        rt[2][i] = rot_left_8( rt[1][i] );
        rt[3][i] = rot_left_8( rt[2][i] );
    }
}


CAES::u32 inline CAES::get( const byte array[4] )
{
#if (('1234' >> 24) == '1')     // little endian ?
    return  *(u32 *)(array);
#else
    return  array[0] <<  0
          | array[1] <<  8
          | array[2] << 16
          | array[3] << 24;
#endif
}


void inline CAES::put( u32 value, byte array[4] )
{
#if (('1234' >> 24) == '1')     // little endian ?
    *(u32 *)(array) = value;
#else
    array[0] = value >>  0 & 0xFF;
    array[1] = value >>  8 & 0xFF;
    array[2] = value >> 16 & 0xFF;
    array[3] = value >> 24       ;
#endif
}


CAES::u32 inline CAES::fb(u32 byte, unsigned int pos)
{
    return ft[pos][ byte >> pos*8 & 0xFF ];
}


CAES::u32 inline CAES::rb(u32 byte, unsigned int pos)
{
    return rt[pos][ byte >> pos*8 & 0xFF ];
}


CAES::u32    CAES::s_box[256], CAES::r_box[256];
CAES::u32    CAES::ft[4][256], CAES::rt[4][256];


//=========================================================================//
// Блок используется при тестировании.
#ifdef AES_TEST
    #ifdef LIBST
        #include "libst.cpp"
        typedef long clock_t;
        clock_t clock(){ return GetTickCount(); }
        const clock_t CLOCKS_PER_SEC = 1000;
        extern "C" int _fltused = 0;
    #else
        #include <string.h>
        #include <stdio.h>
        #include <math.h>
        #include <time.h>
    #endif
#endif
//=========================================================================//


CAES::u32 inline CAES::rot_left_8( u32 value )
{
#if defined(_M_IX86) && !defined(__BORLANDC__)
	return  _rotl(value, 8);                // MSVC и Intel C++ делают ROL   // 'value' is 'unsigned short' in BCB!
#else
    return  value >> 24 | value << 8;
#endif
}


//=========================================================================//
// Блок используется при тестировании.
#ifdef AES_TEST
/** 
 *  Функция используется при компиляции тестового исполняемого файла.
 *  При тестировании проверяется корректность (де)шифрации
 *  и скорость различных методов класса aes при использовании
 *  ключей 128, 192 и 256 бит.
 */
int main_test(void)
{
    /**
     *  Данные для тестирования взяты из fips-197.pdf
     *  http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf
     */
    
    static const byte plaintext[CAES::block_size] = 
    {   
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
    };

    // Для "коротких" ключей используется начальная часть всего ключа.
    static const byte key[] = 
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        // 192 бит
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        // 256 бит
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };

    static const byte ciphertext[3][CAES::block_size] =
    {
        //  для ключа 128 бит
        {0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30, 
         0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a},
        //  для ключа 192 бит
        {0xdd, 0xa9, 0x7c, 0xa4, 0x86, 0x4c, 0xdf, 0xe0,
         0x6e, 0xaf, 0x70, 0xa0, 0xec, 0x0d, 0x71, 0x91},
        //  для ключа 256 бит
        {0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf, 
         0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89}
	};

    aes     crypto;
    byte    buf[crypto.block_size];

	printf("\n AES test:\n");

    for( int i0  = 0; i0 <= 2; i0++ )
    {
        int key_length = 128 + 64 * i0;
        printf("\n %d bit key... ", key_length);

        if( crypto.expand_key(key, key_length) != crypto.Ok )
            printf("error: can't expand key.");
        else
        {
            crypto.encrypt(plaintext, buf);
            if( memcmp(&ciphertext[i0], buf, crypto.block_size) )
                printf("error: encryption failed.");
            else
            {
                crypto.decrypt(buf, buf);
                if( memcmp(plaintext, buf, crypto.block_size) )
                    printf("error: decryption failed.");
                else
                    printf("Ok.");
            }
        }
    }
        
    printf("\n\n Speed test:");
    printf("\n\n   key   | key expansions/sec | encryption, bytes/sec | decryption, bytes/sec");
    for( int i1  = 0; i1 <= 2; i1++ )
    {
        const int times = 1024 * 1024;
        int key_length = 128 + 64 * i1;
        printf("\n %3d bit |", key_length);

        clock_t start = clock();
        for( int t0 = times; t0; t0-- )
        {
            crypto.expand_key(key, key_length);
        }
        printf("    %10d      |",
           (unsigned int)((double)times  * CLOCKS_PER_SEC / (clock() - start))
           );
        
        crypto.encrypt(plaintext, buf);

        start = clock();
        for( int t1 = times; t1; t1-- )
        {
            crypto.encrypt(buf, buf);
        }
        printf("       %10d      |", (unsigned int)
           ((double)crypto.block_size * times * CLOCKS_PER_SEC / (clock() - start))
           );

        start = clock();
        for( int t2 = times; t2; t2-- )
        {
            crypto.decrypt(buf, buf);
        }
        printf("       %10d", (unsigned int)
           ((double)crypto.block_size * times * CLOCKS_PER_SEC / (clock() - start))
           );

        crypto.decrypt(buf, buf);
        if( memcmp(plaintext, buf, crypto.block_size) )
        {
            printf("\n\n error: Monte Carlo test failed.\n");
            return ~0;
        }
    }
    printf("\n\n Ok.\n");    
    return 0;
}
#endif//#ifdef AES_TEST
//=========================================================================//

///========================================================================//
/// EOF
