#include "sdr.h"


/* Generate binary code from binary string 
	binIn <= the input binary string
	nbite <= the number of bits to use in the string (must be less than string length)
	bin   <= the output binary code (-1 for 0, 1 for 1)
	flip  <= with flip=0, the MSB in string binIn will be the 0th index of the output string.
			 But with flip=1, the LSB of binIn string will be the 0th index of the output string.
*/
void bin2bin(const char* binIn, int nbits, char* bin, int flip)
{
	if(flip)
	{
		for(int index=0; index<nbits; index++)
		{
			int index1=nbits-1 - index;
			char ch = binIn[index1];
			bin[index] = ch=='1' ? 1 : -1;
		}
	}
	else
	{
		for(int index=0; index<nbits; index++)
		{
			char ch = binIn[index];
			bin[index] = ch=='1' ? 1 : -1;
		}
	}
}

/* IRNSS data code generator */
EXTERNCPP short* gencode_IRNSS(int prn, int* len, double *crate)
{
	const char* intialconditions[]= { "1110100111",
		"0000100110",
		"1000110100",
		"0101110010",
		"1110110000",
		"0001101011",
		"0000010100" 
	};
	
	if(prn<1 || prn>MAXIRNSSSATNO)
		return NULL;

	short* code = (short*) malloc(sizeof(short) * LEN_IRNSS);
	if(!code)
		return NULL;

	char G1[LEN_IRNSS],G2[LEN_IRNSS],R1[10],R2[10],C1,C2;
    int i,j;

	// initialize the R1 with all zeros
	for (i=0;i<10;i++) R1[i]=-1;
	
	// initialize R2 from the initial condition
	bin2bin(intialconditions[prn-1], 10, R2,1);
    
	// process and generate the chips
	for (i=0;i<LEN_IRNSS;i++) 
	{
        G1[i]=R1[9];
        G2[i]=R2[9];

		// IRNSS
		C1 = R1[2]*R1[9];
		C2 = R2[1]*R2[2]*R2[5]*R2[7]*R2[8]*R2[9];
        for (j=9;j>0;j--)
		{
            R1[j]=R1[j-1];
            R2[j]=R2[j-1];
        }
        
		R1[0]=C1;
        R2[0]=C2;
    }

	// the code chips are inversion of the G2 register content
	// XOR ~G2 with G1, to obtain the final chip code
	for(i=0; i<LEN_IRNSS; i++)
	{
		code[i] = -G2[i]*G1[i];
	}

	*len=LEN_IRNSS;
	*crate=CRATE_IRNSS;

    return code;
}


 