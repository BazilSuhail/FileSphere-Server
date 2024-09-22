#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char *rle_encode(const char *filename, int *encoded_leng) {

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Error opening input file");
        return NULL;
    }
    char *data = NULL;
    size_t buffer_size = 0;
    size_t nread;

    char *encoded = (char *)malloc(sizeof(char) * 1024);
    int encoded_capacity = 1024;
    int encoded_length = 0;
    int len=0;
    while ((nread = getline(&data, &buffer_size, fp)) != -1) {
        // Process the line of data (e.g., remove newline)
        data[strcspn(data, "\n")] = '\0';

        for (int i = 0; i < strlen(data); i++) {
            int count = 1;
            while (i + 1 < strlen(data) && data[i + 1] == data[i]) {
                count++;
                i++;
            }
            len+=count+3;
            if (encoded_length + 4 >= encoded_capacity) {
                // Reallocate buffer if needed
                encoded_capacity *= 2;
                encoded = (char *)realloc(encoded, encoded_capacity);
            }

            if (count < 9) {
                encoded[encoded_length++] = '*';
                encoded[encoded_length++] = '0' + count;
            } else if (count < 100) {
                encoded[encoded_length++] = '#';
                encoded[encoded_length++] = '0' + (count / 10);
                encoded[encoded_length++] = '0' + (count % 10);
            } else if (count < 1000) {
                encoded[encoded_length++] = '@';
                encoded[encoded_length++] = '0' + (count / 100);
                encoded[encoded_length++] = '0' + ((count % 100) / 10);
                encoded[encoded_length++] = '0' + (count % 10);
            } else {
                encoded[encoded_length++] = '$';
                encoded[encoded_length++] = '0' + (count / 1000);
                encoded[encoded_length++] = '0' + ((count % 1000) / 100);
                encoded[encoded_length++] = '0' + ((count % 100) / 10);
                encoded[encoded_length++] = '0' + (count % 10);
            }

            
            encoded[encoded_length++] = data[i];

        }

        // Add newline character
        encoded[encoded_length++] = '*';
        encoded[encoded_length++] = '1';
        encoded[encoded_length++] = '\n';
    }

    fclose(fp);

    *encoded_leng = len;
    return encoded;
}



int main() {
    const char *input_filename = "input.txt";
    const char *output_filename = "output.txt";

    int encoded_length, decoded_length;
    char *encoded = rle_encode(input_filename, &encoded_length);
    if (encoded == NULL) {
        return 1;
    }

    char *decoded = rle_decode(encoded, encoded_length, output_filename);
    if (decoded == NULL) {
        return 1;
    }

    free(encoded);
    free(decoded);

    return 0;
}













// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include<unistd.h>

// struct RLE{
//     char ch;
//     int count;
// };


// struct RLE *rle_encode(const char *filename, int *encoded_length) {
//     FILE *fp = fopen(filename, "r");
//     if (fp == NULL) {
//         perror("Error opening input file");
//         return NULL;
//     }

//     char *data = NULL;
//     size_t buffer_size = 0;
//     ssize_t nread;

//     struct RLE* encoded = (struct RLE *)malloc(sizeof(struct RLE) * 1024); 
//     int encoded_capacity = 1024;
//     int encoded_leng = 0;
//     int length=0;

//     while ((nread = getline(&data, &buffer_size, fp)) != -1) {
        
//         data[strcspn(data, "\n")] = '\0';

//         for (int i = 0; data[i] != '\0'; i++) {
//             int count = 1;
//             while (i + 1 < strlen(data) && data[i + 1] == data[i]) {
//                 count++;
//                 i++;
//             }
//             length+=count+1;
//             if (encoded_leng + 2 >= encoded_capacity) {
//                 // Reallocate buffer if needed
//                 encoded_capacity *= 2;
//                 encoded = (struct RLE *)realloc(encoded, encoded_capacity);
//             }
//             encoded[encoded_leng].ch = data[i];
//             encoded[encoded_leng].count=count;
//             encoded_leng++;
//         }
//         encoded[encoded_leng].ch = '\n';
//         encoded[encoded_leng].count=1;
//         encoded_leng++;
//     }
    
//     fclose(fp);

//     *encoded_length = length;
//     return encoded;
// }

// char *rle_decode(const struct RLE *encoded_data, int encoded_length, const char *output_filename) {
//     printf("%d",encoded_length);
//     char *decoded = (char *)malloc(sizeof(char) * encoded_length);
//     int i, j = 0;

//     for (i = 0; i < encoded_length; i++) {
//         for (int k = 0; k < encoded_data[i].count; k++)
//         {
//             decoded[j++]=encoded_data[i].ch;
//         } 
//     }
//     FILE *fp = fopen(output_filename, "w");
//     if (fp == NULL) {
//         perror("Error opening output file");
//         return NULL;
//     }
//     if(j>1024)
//     {
//        int quo=j/1024;
//        j=j%1024;
//        int Buffer =1024;
//        for (int i = 0; i < quo; i++)
//        {
//           fwrite(decoded, sizeof(char), Buffer, fp);
//        }
//     }
//     fwrite(decoded, sizeof(char), j, fp);

    
//     fclose(fp);

//     return decoded;
// }

// int main() {
//     const char *input_filename = "input.txt";
//     const char *output_filename = "output.txt";

//     int encoded_length, decoded_length;
//     struct RLE* encoded = rle_encode(input_filename, &encoded_length);
//     if (encoded == NULL) {
//         return 1;
//     }

//     char *decoded = rle_decode(encoded, encoded_length, output_filename);
//     if (decoded == NULL) {
//         return 1;
//     }

//     free(encoded);
//     free(decoded);

//     return 0;
// }










// // #include <stdio.h>
// // #include <stdlib.h>

// // char *rle_encode(char *data, int length, int *encoded_length) {
// //     char *encoded = (char *)malloc(sizeof(char) * length * 2);
// //     int i, j = 0;

// //     for (i = 0; i < length; i++) {
// //         encoded[j++] = data[i];
// //         int count = 1;
// //         while (i + 1 < length && data[i + 1] == data[i]) {
// //             count++;
// //             i++;
// //         }
// //         if (count > 1) {
// //             if (count >= 10) {
// //                 // Handle multiple-digit counts
// //                 int digits = 0;
// //                 int temp = count;
// //                 while (temp > 0) {
// //                     digits++;
// //                     temp /= 10;
// //                 }
// //                 for (int k = 0; k < digits; k++) {
// //                     encoded[j++] = '0' + (count % 10);
// //                     count /= 10;
// //                 }
// //             } else {
// //                 encoded[j++] = '0' + count;
// //             }
// //         }
// //     }

// //     *encoded_length = j;
// //     return encoded;
// // }

// // char *rle_decode(char *encoded, int encoded_length, int *decoded_length) {
// //     char *decoded = (char *)malloc(sizeof(char) * encoded_length);
// //     int i, j = 0;
// //     for (i = 0; i < encoded_length; i++) {

// //         printf("%c",encoded[i]);
// //         char ch=encoded[i];
// //         if (i + 1 < encoded_length && '0' <= encoded[i + 1] && encoded[i + 1] <= '9') {
           
// //             int count = encoded[i + 1] - '0';
// //              i++;
// //             while (i + 1 < encoded_length && '0' <= encoded[i + 1] && encoded[i + 1] <= '9') {
// //                 count = count * 10 + (encoded[i + 1] - '0');
// //                 i++;
// //             }
// //             for (int k = 0; k < count; k++)
// //             {
// //                 printf("%c",ch);
// //             }
// //         }
// //     }

// //     *decoded_length = j;
// //     return decoded;
// // }


// // int main() {
// //     char data[] = "aaaaaaaaaaabbccc";
// //     int length = strlen(data);
// //     int encoded_length,decoded_length;
// //     char *encoded = rle_encode(data, length, &encoded_length);
// //     printf("Encoded data: %s\n", encoded);

// //     char *decoded = rle_decode(encoded, encoded_length, &decoded_length);
// //     printf("Decoded data: %s\n", decoded);

// //     free(encoded);
// //     free(decoded);

// //     return 0;
// // }