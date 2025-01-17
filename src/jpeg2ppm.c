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

    if (argc != 2) {
        /*
          #  Si y'a pas au moins un argument en ligne de commandes, on
          #  boude.
        */
        fprintf(stderr, "Usage: %s fichier.jpeg\n", argv[0]);
        return EXIT_FAILURE;
    }
    // choix de l'image à décoder
    struct data *d = decode_entete(argv[1]);

    // nombre de bloc par lignes et par colonnes
    int16_t nb_block_ligne = d->image_width / 8 + ((d->image_width % 8 != 0) ? 1 : 0);
    int16_t nb_block_colonne = d->image_height / 8 + ((d->image_height % 8 != 0) ? 1 : 0);
    
    // initialisation des coefficients DC
    int16_t precDC = 0;
    int16_t precDC_Cb = 0;
    int16_t precDC_Cr = 0;

    // fichier de sortie
    const char *filename = argv[1];
    int16_t cpt = 0;

    while(filename[cpt] != '/'){
        cpt++;
    }

    
    while(filename[cpt] != '.'){
        cpt ++;
    }
    char *name = malloc((cpt + 1) * sizeof(char *));
    memcpy(name, filename, cpt + 1);
    name[cpt + 1] = '\0';

    char *result_final = malloc((cpt + 4)*sizeof(char*));
    memcpy(result_final, name, cpt + 1);

    
    // Image en noir et blanc
    if(d->nb_component_scan == 1){
        
        // Ouverture du fichier
        memcpy(result_final + cpt + 1, "pgm", 3);
        result_final[cpt +  4] = '\0';
        FILE *image = fopen(result_final, "wb");

        // Création de l'entête de l'image
        create_pgm_header(image, d->image_width, d->image_height);

        // Allocation de mémoire pour tous les pointeurs dont nous aurons besoin durant le décodage
         uint8_t ***mat=malloc(nb_block_ligne*sizeof(uint8_t **));
        for(int16_t i = 0; i < nb_block_ligne; i++){
            mat[i] = malloc(8*sizeof(uint8_t *));
            for(int16_t j = 0; j < 8; j++){
                mat[i][j] = malloc(8*sizeof(uint8_t));
            }
        }
        
        int16_t *block = malloc(64 * sizeof (int16_t));

        uint8_t **S=malloc(8*sizeof(uint8_t*));
        for(int8_t x=0; x<8; x++){
            S[x]=malloc(8*sizeof(uint8_t));
        }

        int16_t **matrice = malloc(8*sizeof(int16_t*));
        for(int8_t i=0; i<8; i++){
            matrice[i]=malloc(8*sizeof(int16_t));
        }

        // pour chaque bloc, on effectue les étapes du décodage
        for(int16_t k = 0; k < nb_block_colonne; k++){

            for (int16_t i = 0; i < nb_block_ligne; i++){

                // décodage des coefficients AC/DC
                decode_ac_dc(d,0,1,d->file,block);
                decode_ac_dc(d,0,0,d->file,block);
                block[0] = block[0] + precDC;
                precDC = block[0];

                // quantification inverse
                quantification_inverse(d,0,block);

                // zigzag inverse
                zig_zag(block, matrice);
                
                // iDCT
                iDCT_rapide(matrice, S);
                
                for (int x = 0; x< 8 ; x++){
                    for (int y = 0; y< 8 ; y++){
                        mat[i][x][y] = S[x][y];
                    }
                }

            }
            // création du fichier pgm
            create_pgm(image ,mat, d->image_width, d->image_height);
        }
        // libération de la mémoire 
        fclose(image);
        free(name);
        free(result_final);
        for(int16_t i = 0; i < nb_block_ligne; i++){
            for(int16_t j = 0; j < 8; j++){
               free(mat[i][j]);
            }
            free(mat[i]);
        }
        free(mat);
        free(block);
        for(int i = 0; i < 8 ; i ++){
            free(S[i]);
            free(matrice[i]);
        }
        free(S);
        free(matrice);

        for (int i = 0; i < 4; i++) {
            if(d->quantization_table_read[i] != NULL){
                free(d->quantization_table_read[i]);
            }
        }
        
        free(d->quantization_table_read);
          
        free(d->list_component);
        free(d->list_scan_components);
       
        for(int8_t i = 0; i < d->nb_dc; i++ ){
            if(d->list_dc[i].huff_values != NULL){
                free(d->list_dc[i].huff_values);
                free_huff(d->list_dc[i].racine_huffman);
                
            }
        }
        free(d->list_dc);
        

        for(int8_t i = 0; i < d->nb_ac; i++ ){
            if(d->list_ac[i].huff_values != NULL){
                free(d->list_ac[i].huff_values);
                free_huff(d->list_ac[i].racine_huffman);
            }
        }
        free(d->list_ac);


        fclose(d->file);
        free(d);
        


}
    // Image en couleur
    else{

        // Ouverture du fichier 
        memcpy(result_final + cpt + 1, "ppm", 3);
        result_final[cpt +  4] = '\0';
        FILE *image = fopen(result_final, "wb");

        // Création de l'entête de l'image
        create_ppm_header(image, d->image_width, d->image_height);

        // récupération des facteurs d'échantillonnages
        struct component *comp = d->list_component;
        int8_t sampling_w = comp->sampling_horizontal;
        int8_t sampling_h = comp->sampling_vertical;

        // Récupération du nombre de block par ligne
        nb_block_ligne = d->image_width / (8 * sampling_w) + ((d->image_width % (8 * sampling_w) != 0) ? 1 : 0);

        // Allocation de mémoire pour tous les pointeurs dont nous aurons besoin durant le décodage
        uint8_t ***red=malloc(nb_block_ligne*sampling_w*sizeof(uint8_t **));
        for(int16_t i = 0; i < nb_block_ligne*sampling_w; i++){
            red[i] = malloc(8*sampling_h*sizeof(uint8_t *));
            for(int16_t j = 0; j < 8*sampling_h; j++){
                red[i][j] = malloc(8*sampling_w*sizeof(uint8_t));
            }
        }
        uint8_t ***green=malloc(nb_block_ligne*sampling_w*sizeof(uint8_t **));
        for(int16_t i = 0; i < nb_block_ligne*sampling_w; i++){
            green[i] = malloc(8*sampling_h*sizeof(uint8_t *));
            for(int16_t j = 0; j < 8*sampling_h; j++){
                green[i][j] = malloc(8*sampling_w*sizeof(uint8_t));
            }
        }
        uint8_t ***blue=malloc( nb_block_ligne*sampling_w*sizeof(uint8_t **));
        for(int16_t i = 0; i < nb_block_ligne*sampling_w; i++){
            blue[i] = malloc(8*sampling_h*sizeof(uint8_t *));
            for(int16_t j = 0; j < 8*sampling_h; j++){
                blue[i][j] = malloc(8*sampling_w*sizeof(uint8_t));
            }
        }
        int16_t **Y = malloc((sampling_h * sampling_w)*sizeof(int16_t *));
        for(int16_t i = 0; i < sampling_h * sampling_w; i++){
            Y[i] = malloc(64 * sizeof(int16_t));
        }
        int16_t *block_Cb = malloc(sizeof (int16_t) * 64);
        int16_t *block_Cr = malloc(sizeof (int16_t) * 64);

        uint8_t **new_Y = malloc(8*sampling_h * sizeof(uint8_t *));
        for(int8_t i = 0; i < 8 * sampling_h; i++){
            new_Y[i] = malloc(8 * sampling_w * sizeof(uint8_t));
        }

        int16_t ***matrice_Y = malloc((sampling_h * sampling_w)*sizeof(int16_t **));
        for (int16_t i = 0; i < sampling_h * sampling_w; i++){
            matrice_Y[i] = malloc(8*sizeof(int16_t*));
            for(int8_t j=0; j<8; j++){
                matrice_Y[i][j]=malloc(8*sizeof(int16_t));
            }
        }
        uint8_t ***pixel_Y = malloc((sampling_h * sampling_w)*sizeof(uint8_t **));
        
        for(int8_t i = 0; i<sampling_w*sampling_h; i++){
            pixel_Y[i] = malloc(8*sizeof(uint8_t *));
            for(uint8_t j=0; j<8; j++){
                pixel_Y[i][j]= malloc(8*sizeof(uint8_t));
            }
        }

        uint8_t **pixel_Cb=malloc(8*sizeof(uint8_t*));
        for(int8_t x=0; x<8; x++){
            pixel_Cb[x]=malloc(8*sizeof(uint8_t));
        }

        uint8_t **pixel_Cr=malloc(8*sizeof(uint8_t*));
        for(int8_t x=0; x<8; x++){
            pixel_Cr[x]=malloc(8*sizeof(uint8_t));
        }

        uint8_t **new_Cb = malloc(8*sampling_h*sizeof(uint8_t*));
        for(int8_t i = 0; i<8*sampling_h; i++){
            new_Cb[i] = malloc(8*sampling_w*sizeof(uint8_t));
        }

        uint8_t **new_Cr = malloc(8*sampling_h*sizeof(uint8_t*));
        for(int8_t i = 0; i<8*sampling_h; i++){
            new_Cr[i] = malloc(8*sampling_w*sizeof(uint8_t));
        }

        int16_t **matrice_Cb = malloc(8*sizeof(int16_t*));
        for(int8_t i=0; i<8; i++){
            matrice_Cb[i]=malloc(8*sizeof(int16_t));
        }

        int16_t **matrice_Cr = malloc(8*sizeof(int16_t*));
        for(int8_t i=0; i<8; i++){
            matrice_Cr[i]=malloc(8*sizeof(int16_t));
        }
     
        for(int16_t k = 0; k < nb_block_colonne/sampling_h; k++){
            
            for(int16_t i = 0; i < nb_block_ligne; i++){

                // décodage des coefficients AC/DC
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

                // quantification inverse
                for(int16_t i = 0; i < sampling_h * sampling_w; i++){
                    quantification_inverse(d,d->list_component[0].quantization_table_index,Y[i]);
                }
               
                
                quantification_inverse(d,d->list_component[1].quantization_table_index,block_Cb);
                
                quantification_inverse(d,d->list_component[2].quantization_table_index,block_Cr);

                // zig zag inverse
                for(int16_t i = 0; i < sampling_h*sampling_w; i++){
                    zig_zag_rapide(Y[i], matrice_Y[i]);
                }
               
                zig_zag_rapide(block_Cb, matrice_Cb);
              
                 
                zig_zag_rapide(block_Cr, matrice_Cr);

                // iDCT
                for(int16_t i = 0; i < sampling_h*sampling_w; i++){
                    iDCT_rapide(matrice_Y[i], pixel_Y[i]);
                }
               

                iDCT_rapide(matrice_Cb, pixel_Cb);
              
                iDCT_rapide(matrice_Cr, pixel_Cr);

                 
                for(int x= 0 ; x < sampling_h; x++){
                    for(int i=0; i< 8 ;i++){
                        for(int j=0; j < 8*sampling_w; j++){
                            new_Y[(x * 8) + i ][j] = pixel_Y[(x * sampling_w) + j/8][i][j%8];
         
                        }                  
                    }
                }
                // Conversion en RGB
                if( sampling_h != 1 || sampling_w != 1){
                    sur_ech(pixel_Cb, d, new_Cb);
                    sur_ech(pixel_Cr, d, new_Cr);
                   YCbCr_to_RGB(new_Y, new_Cb, new_Cr, d, red[i], green[i], blue[i]);
                }
                else{
                  YCbCr_to_RGB(new_Y, new_Cb, new_Cr, d, red[i], green[i], blue[i]);
                }   
            }    
            // création du fichier ppm
            create_ppm(image, red, green , blue, d);
        }
        
        // libération de la mémoire

        fclose(image); 
        free(name);
        free(result_final);

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
        for(int16_t i = 0; i < sampling_h * sampling_w; i++){
            free(Y[i]);
        }
        free(Y);
            
        free(block_Cb);
        free(block_Cr);
        for(int i = 0; i < 8*sampling_h ; i ++){
            free(new_Y[i]);
            free(new_Cb[i]);
            free(new_Cr[i]);
        }
        free(new_Y);
        free(new_Cb);
        free(new_Cr);
         
        for (int16_t i = 0; i < sampling_h * sampling_w; i++){
            for(int8_t j=0; j<8; j++){
                free(matrice_Y[i][j]);
                free(pixel_Y[i][j]);
            }
            free(matrice_Y[i]);
            free(pixel_Y[i]);
        }
        free(matrice_Y);
        free(pixel_Y);
       
        for(int8_t i=0; i<8; i++){
            free(pixel_Cb[i]);
            free(pixel_Cr[i]);
            free(matrice_Cb[i]);
            free(matrice_Cr[i]);

        }
        free(pixel_Cb);
        free(pixel_Cr);
        free(matrice_Cb);
        free(matrice_Cr);

        
        for (int i = 0; i < 4; i++) {
            if(d->quantization_table_read[i] != NULL){
                free(d->quantization_table_read[i]);
            }
        }
        
        free(d->quantization_table_read);
          
        free(d->list_component);
        free(d->list_scan_components);
       
        for(int8_t i = 0; i < d->nb_dc; i++ ){
            if(d->list_dc[i].huff_values != NULL){
                free(d->list_dc[i].huff_values);
                free_huff(d->list_dc[i].racine_huffman);
                
            }
        }
        free(d->list_dc);
        for(int8_t i = 0; i < d->nb_ac; i++ ){
            if(d->list_ac[i].huff_values != NULL){
                free(d->list_ac[i].huff_values);
                free_huff(d->list_ac[i].racine_huffman);
            }
        }
        free(d->list_ac);
        fclose(d->file);
        free(d);
    }
    return EXIT_SUCCESS;
}
