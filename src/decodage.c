#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "../include/decode_entete.h"
#include "../include/huffman.h"
#include "../include/extract_bloc.h"
#include "../include/iDCT.h"
#include "../include/couleur_matrice.h"
#include "../include/ppm.h"
#include "../include/upsampling.h"
#include "../include/quantification_inverse.h"
#include "../include/zig_zag.h"


int main(int argc, char **argv){
    struct data *d = decode_entete("../images/zig-zag.jpg");

  
    int16_t nb_block_ligne = d->image_width / 8 + ((d->image_width % 8 != 0) ? 1 : 0);
    int16_t nb_block_colonne = d->image_height / 8 + ((d->image_height % 8 != 0) ? 1 : 0);
    
    int16_t precDC = 0;
    int16_t precDC_Cb = 0;
    int16_t precDC_Cr = 0;

    FILE *test_invaders = fopen("zig-zag.ppm", "wb");

    if(d->nb_component_scan == 1){
        uint8_t ***mat=malloc(nb_block_ligne*sizeof(uint8_t **));
        create_pgm_header(test_invaders, d->image_width, d->image_height);

        int16_t *block = malloc(sizeof (int16_t) * 64);

        for(int16_t k = 0; k < nb_block_colonne; k++){

            for (int16_t i = 0; i < nb_block_ligne; i++){

                decode_ac_dc(d,0,1,d->file,block);
                decode_ac_dc(d,0,0,d->file,block);
                block[0] = block[0] + precDC;
                precDC = block[0];

                int16_t * data = quantification_inverse(d,0,block);

                int16_t ** matrice = zig_zag(data);
                
                uint8_t **pixel= iDCT(matrice);
                
                mat[i]=pixel;

            }
            create_pgm(test_invaders ,mat, d->image_width, d->image_height);
        }
        fclose(test_invaders);
        free(mat);
        free(block);
}
    else{
        
        create_ppm_header(test_invaders, d->image_width, d->image_height);
        struct component *comp = d->list_component;
        int8_t sampling_w = comp->sampling_horizontal;
        int8_t sampling_h = comp->sampling_vertical;

        nb_block_ligne = d->image_width / (8 * sampling_w) + ((d->image_width % (8 * sampling_w) != 0) ? 1 : 0);

        uint8_t ***red=malloc(nb_block_ligne*sampling_w*sizeof(uint8_t **));
        uint8_t ***green=malloc(nb_block_ligne*sampling_w*sizeof(uint8_t **));
        uint8_t ***blue=malloc( nb_block_ligne*sampling_w*sizeof(uint8_t **));
        int16_t **Y = malloc((sampling_h * sampling_w)*sizeof(int16_t *));
        int16_t *block_Cb = malloc(sizeof (int16_t) * 64);
        int16_t *block_Cr = malloc(sizeof (int16_t) * 64);

        uint8_t **new_Y = malloc(8*sampling_h * sizeof(uint8_t *));
        for(int8_t i = 0; i < 8 * sampling_h; i++){
            new_Y[i] = malloc(8 * sampling_w * sizeof(uint8_t));
        }

        int16_t ***matrice_Y = malloc((sampling_h * sampling_w)*sizeof(int16_t **));
        uint8_t ***pixel_Y = malloc((sampling_h * sampling_w)*sizeof(uint8_t **));

        uint8_t **R=malloc(8*sampling_h*sizeof(uint8_t *));
        for(uint8_t i=0; i<8*sampling_h; i++){
            R[i]=malloc(8*sampling_w*sizeof(uint8_t));
        }
        uint8_t **B=malloc(8*sampling_h*sizeof(uint8_t *));
        for(uint8_t i=0; i<8*sampling_h; i++){
            B[i]=malloc(8*sampling_w*sizeof(uint8_t));
        }
        uint8_t **G=malloc(8*sampling_h*sizeof(uint8_t *));
        for(uint8_t i=0; i<8*sampling_h; i++){
            G[i]=malloc(8*sampling_w*sizeof(uint8_t));
        }

        for(int16_t k = 0; k < nb_block_colonne/sampling_h; k++){
            for(int16_t i = 0; i < nb_block_ligne; i++){
            
                for(int16_t i = 0; i < sampling_h * sampling_w; i++){
                    Y[i] = malloc(64 * sizeof(int16_t));
                }

                for(int16_t i = 0; i < sampling_h * sampling_w; i++){
                    decode_ac_dc(d,d->list_scan_components[0].associated_dc_huffman_table_index,1,d->file,Y[i]);
                    decode_ac_dc(d,d->list_scan_components[0].associated_ac_huffman_table_index,0,d->file,Y[i]);

                    Y[i][0] = Y[i][0] + precDC;
                    precDC = Y[i][0];
                }
              

                decode_ac_dc(d,d->list_scan_components[1].associated_dc_huffman_table_index,1,d->file,block_Cb);
                decode_ac_dc(d,d->list_scan_components[1].associated_ac_huffman_table_index,0,d->file,block_Cb);
                block_Cb[0] = block_Cb[0] + precDC_Cb;
                precDC_Cb = block_Cb[0];
                  
                decode_ac_dc(d,d->list_scan_components[2].associated_dc_huffman_table_index,1,d->file,block_Cr);
                decode_ac_dc(d,d->list_scan_components[2].associated_ac_huffman_table_index,0,d->file,block_Cr);
                block_Cr[0] = block_Cr[0] + precDC_Cr;
                precDC_Cr = block_Cr[0];


                for(int16_t i = 0; i < sampling_h * sampling_w; i++){
                    Y[i] = quantification_inverse(d,d->list_component[0].quantization_table_index,Y[i]);
                }
               
                
                int16_t * data_Cb = quantification_inverse(d,d->list_component[1].quantization_table_index,block_Cb);
                
                int16_t * data_Cr = quantification_inverse(d,d->list_component[2].quantization_table_index,block_Cr);

                for(int16_t i = 0; i < sampling_h*sampling_w; i++){
                    matrice_Y[i] = zig_zag(Y[i]);
                }
                
                int16_t ** matrice_Cb = zig_zag(data_Cb);
                
                int16_t ** matrice_Cr = zig_zag(data_Cr);

                for(int16_t i = 0; i < sampling_h*sampling_w; i++){
                    pixel_Y[i] = iDCT(matrice_Y[i]);
                }
                uint8_t **pixel_Cb= iDCT(matrice_Cb);
                uint8_t **pixel_Cr= iDCT(matrice_Cr);
                
                
                for(int x= 0 ; x < sampling_h; x++){
                    for(int i=0; i< 8 ;i++){
                        for(int j=0; j < 8*sampling_w; j++){
                            new_Y[(x * 8) + i ][j] = pixel_Y[(x * sampling_w) + j/8][i][j%8];
                        }
                    }
                }
                
                if( sampling_h != 1 || sampling_w != 1){
                    uint8_t **new_Cb = sur_ech(pixel_Cb, d);
                    uint8_t **new_Cr = sur_ech(pixel_Cr, d);
                    red[i] = YCbCr_to_R(new_Y, new_Cb, new_Cr, d, R);
                    green[i] = YCbCr_to_G(new_Y, new_Cb, new_Cr, d, G);
                    blue[i] = YCbCr_to_B(new_Y, new_Cb, new_Cr, d, B);
                }
                else{
                    red[i] = YCbCr_to_R(new_Y, pixel_Cb, pixel_Cr, d, R);
                    green[i] = YCbCr_to_G(new_Y, pixel_Cb, pixel_Cr, d, G);
                    blue[i] = YCbCr_to_B(new_Y, pixel_Cb, pixel_Cr, d, B);
                }
               
            }    
            
            create_ppm(test_invaders, red, green , blue, d);
        }
        fclose(test_invaders); 

        for(int i = 0; i < nb_block_ligne*sampling_w ; i ++){
            for(uint8_t j=0; j<8*sampling_h; j++){
                free(red[i][j]);
                free(green[i][j]);
                free(blue[i][j]);
            }
            free(red[i]);
            free(blue[i]);
            free(green[i]);
        }
        free(red);
        free(green);
        free(blue);
        free(Y);
       
        free(block_Cb);
        free(block_Cr);
        for(int i = 0; i < 8*sampling_h ; i ++){
            free(new_Y[i]);
        }
        free(new_Y);
        for(int i = 0; i < 8*sampling_h ; i ++){
            free(new_Y[i]);
        }
        free(matrice_Y);
        free(pixel_Y);
        free(d->quantization_index);
        for (int i = 0; i < 4; i++) {
            free(d->quantization_table_read[i]);
        }
        free(d->quantization_table_read);
        for(int8_t i = 0; i < d->nb_of_component; i++){
            free(&d->list_component[i]);
        }
        free(d->list_component);
        /*
        for(int8_t i = 0; i < 4; i++ ){
            free(d->list_dc[i].huff_values);
            free_huff(d->list_dc[i].racine_huffman);
            free(&d->list_dc[i]);
        }
        free(d->list_dc);


        for(int8_t i = 0; i < 4; i++ ){
            if(d->list_dc[i].huff_values != NULL){
                free(d->list_ac[i].huff_values);
                free_huff(d->list_ac[i].racine_huffman);
                free(&d->list_ac[i]);
            }
        }
        free(d->list_ac);
        */


    
        free(d);
        for(int i = 0; i < 8*sampling_h ; i ++){
            free(R[i]);
        }
        free(R);
        for(int i = 0; i < 8*sampling_h ; i ++){
            free(G[i]);
        }
        free(G);
        for(int i = 0; i < 8*sampling_h ; i ++){
            free(B[i]);
        }
        free(B);

    } 

    return EXIT_SUCCESS;
}
