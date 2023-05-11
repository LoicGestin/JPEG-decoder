#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "../include/decode_entete.h"
#include "../include/huffman.h"
#include "../include/extract_bloc.h"
#include "../include/iDCT.h"
#include "../include/ppm.h"
#include "../include/upsampling.h"
#include "../include/quantification_inverse.h"
#include "../include/zig_zag.h"


int main(int argc, char **argv)
{
    struct data *d = decode_entete("../images/bisou.jpeg");

    printf("%x \n", d->byte);
    int16_t cpt =0;
    int16_t nb_block_ligne = d->image_width / 8 + ((d->image_width % 8 != 0) ? 1 : 0);
    int16_t nb_block_colonne = d->image_height / 8 + ((d->image_height % 8 != 0) ? 1 : 0);
    uint8_t ***mat=malloc(nb_block_ligne*sizeof(uint8_t **));
    int16_t precDC = 0;
    FILE *test_invaders = fopen("bisou.pgm", "wb");
    create_pgm_header(test_invaders, d->image_width, d->image_height);
    for(int16_t k = 0; k < nb_block_colonne; k++){
        for (int16_t i = 0; i < nb_block_ligne; i++){
            int16_t *block = malloc(sizeof (int16_t) * 64);
            //fread(&d->byte, 1, 1, d->file);
            decode_ac_dc(d,0,1,d->file,block);
            decode_ac_dc(d,0,0,d->file,block);
            block[0] = block[0] + precDC;
            precDC = block[0];
            printf("\n\n");
            int16_t * data = quantification_inverse(d,0,block);

            for(int8_t i = 0; i < 64; i++){
                printf("%hx ", data[i]);
            }
            printf("\n\n");
            int16_t ** matrice = zig_zag(data);
            for(int8_t x = 0; x < 8; x++){
                for(int8_t j = 0; j < 8; j++){
                    printf("%hx ", matrice[x][j]);
                }
                printf("\n");
            }
            printf("\n\n");
            
            
            printf("\n");
            uint8_t **pixel= iDCT(matrice);
            
            for(int8_t x = 0; x < 8; x++){
                for(int8_t j = 0; j < 8; j++){
                    printf("%hx ", pixel[x][j]);
                }
                printf("\n");
            }
            
            cpt++;
            mat[i]=pixel;
            free(block);
            printf("%d\n", cpt);
            printf("%d\n", nb_block_ligne);
            printf("%d\n", nb_block_colonne);
        }
        create_pgm(test_invaders ,mat, d->image_width, d->image_height);
    }
    fclose(test_invaders);



    /*
    for(int8_t i=0; i < nb_block; i++){
        int16_t *block = malloc(sizeof (int16_t) * 64);
        //fread(&d->byte, 1, 1, d->file);
        decode_ac_dc(d,0,1,d->file,block);
        decode_ac_dc(d,0,0,d->file,block);
        printf("\n\n");
        int16_t * data = quantification_inverse(d,0,block);

        for(int8_t i =0; i < 64; i++){
            printf("%hx ", data[i]);
        }
        printf("\n\n");
        int16_t ** matrice = zig_zag(data);
        for(int8_t k =0; k < 8; k++){
            for(int8_t j =0; j < 8; j++){
                printf("%hx ", matrice[k][j]);
            }
            printf("\n");
        }
        printf("\n\n");

        
        printf("\n");
        uint8_t **pixel= iDCT(matrice);


        for(int8_t k =0; k < 8; k++){
            for(int8_t j =0; j < 8; j++){
                printf("%02x ", pixel[k][j]);
            }
            printf("\n");
        }
         create_pgm("test_invaders.pgm",pixel,8,8);
    }*/
   



    return EXIT_SUCCESS;
}