#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../include/huffman.h"
#include "../include/decode_entete.h"

void insert_code_huffman(struct cellule_huffman **c,int value, int length, BYTE symbol){
    if (*c == NULL) {
        *c = malloc(sizeof(struct cellule_huffman));
        (*c)->symbol = -1;
        (*c)->left = NULL;
        (*c)->right = NULL;
    }
    if (length <= 1) {
        (*c)->symbol = symbol;
    }
    else {
        int is_bit_set = (value >> (length - 2)) & 1;
        if (is_bit_set) {
            insert_code_huffman(&((*c)->right), value, length - 1,symbol );
        } else {
            insert_code_huffman(&((*c)->left), value, length - 1, symbol);
        }
    }
}

void display_huffman_tree(struct cellule_huffman* c, char* path) {
    if (c == NULL) {
        return;
    }

    if (c->right == NULL && c->left == NULL && strlen(path) > 0) {
        printf("   path: %s, symbol: %x\n", path,c->symbol);
    } else {
        char* left_path = malloc(strlen(path)+  1);
        strcpy(left_path, path);
        strcat(left_path, "0");

        char* right_path = malloc(strlen(path)+ 1);
        strcpy(right_path, path);
        strcat(right_path, "1");

        display_huffman_tree(c->left, left_path);
        display_huffman_tree(c->right, right_path);

       // free(left_path);
       // free(right_path);
    }
}
void decode_huffman(struct data* d, int index, int table_type){

    struct dht_ac_dc *current_dht = table_type ? &d->list_dc[index] :  &d->list_ac[index];
    //current_dht->huff_code = malloc(current_dht->nb_symbols * sizeof(int));
    //current_dht->huff_length = malloc(current_dht->nb_symbols * sizeof(int));
    current_dht->racine_huffman = malloc(sizeof(struct cellule_huffman));
    current_dht->racine_huffman->symbol = -1;
    current_dht->racine_huffman->right = NULL;
    current_dht->racine_huffman->left = NULL;

    int ind = 0;
    int value = 0;
    int length = 1;
    for(int i=0; i<16; i ++){

        for(int j=0; j < current_dht->nb_code[i]; j++){
            //current_dht->huff_code[ind] = value;
            //current_dht->huff_length[ind] = length;
            insert_code_huffman(&current_dht->racine_huffman,value,length+1,current_dht->huff_values[ind]);
            value ++;
            ind ++;
        }
        value <<=1;
        length ++;
    }


}