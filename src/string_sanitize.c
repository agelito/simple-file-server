// string_sanitize.c

static char valid_filename_characters[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 
    'u', 'v', 'w', 'x', 'y', 'z',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 
    'U', 'V', 'W', 'X', 'Y', 'Z', 
    '.', '_', '-'
};

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))

char
sanitize_character(char input)
{
    int valid = 0;
    
    int i = 0;
    int valid_count = ARRAY_LENGTH(valid_filename_characters);
    for(i = 0; i < valid_count; ++i)
    {
        if(valid_filename_characters[i] == input)
        {
            valid = 1;
            break;
        }
    }

    if(!valid)
    {
        input = '_';
    }

    return input;
}

void
santitize_file_name(char* input, int input_length, char* output, int output_length)
{
    if(input_length >= output_length)
        input_length = output_length - 1;

    int i;
    for(i = 0; i < input_length; ++i)
    {
        char input_character = *(input + i);
        *(output + i) = sanitize_character(input_character);
    }

    *(output + input_length) = 0;
}
