#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

// Simplified ARM instruction set and their corresponding opcodes/formats
// This is a highly simplified representation for demonstration purposes.
// A real ARM assembler would be much more complex.

// Instruction types for operand parsing
#define TYPE_DATA_PROCESSING_IMM 0 // Rd, Rn, #imm
#define TYPE_DATA_PROCESSING_REG 1 // Rd, Rn, Rm
#define TYPE_BRANCH              2 // label
#define TYPE_SWI                 3 // #imm
#define TYPE_MOV_IMM             4 // Rd, #imm
#define TYPE_MOV_REG             5 // Rd, Rm
#define TYPE_MUL                 6 // Rd, Rm, Rs
#define TYPE_CMP_REG             7 // Rn, Rm
#define TYPE_SHIFT_IMM           8 // Rd, Rm, #imm
#define TYPE_CMP_IMM             9 // Rn, #imm

typedef struct
{
  char *mnemonic;
  unsigned int opcode_base;        // Base opcode for the instruction
  int type;                        // Defines how operands are parsed and encoded
} Instruction;

Instruction instructions[] =
{
    {"MOV", 0xE3A00000, TYPE_MOV_IMM},             // MOV Rd, #imm
    {"MOV", 0xE1A00000, TYPE_MOV_REG},             // MOV Rd, Rm
    {"ADD", 0xE0800000, TYPE_DATA_PROCESSING_REG}, // ADD Rd, Rn, Rm
    {"ADD", 0xE2800000, TYPE_DATA_PROCESSING_IMM}, // ADD Rd, Rn, #imm
    {"SUB", 0xE0400000, TYPE_DATA_PROCESSING_REG}, // SUB Rd, Rn, Rm
    {"SUB", 0xE2400000, TYPE_DATA_PROCESSING_IMM}, // SUB Rd, Rn, #imm
    {"CMP", 0xE1500000, TYPE_CMP_REG},             // CMP Rn, Rm
    {"CMP", 0xE3500000, TYPE_CMP_IMM},             // CMP Rn, #imm
    {"AND", 0xE0000000, TYPE_DATA_PROCESSING_REG}, // AND Rd, Rn, Rm
    {"AND", 0xE2000000, TYPE_DATA_PROCESSING_IMM}, // AND Rd, Rn, #imm
    {"ORR", 0xE1800000, TYPE_DATA_PROCESSING_REG}, // ORR Rd, Rn, Rm
    {"ORR", 0xE3800000, TYPE_DATA_PROCESSING_IMM}, // ORR Rd, Rn, #imm
    {"EOR", 0xE0200000, TYPE_DATA_PROCESSING_REG}, // EOR Rd, Rn, Rm
    {"EOR", 0xE2200000, TYPE_DATA_PROCESSING_IMM}, // EOR Rd, Rn, #imm
    {"LSL", 0xE1A00000, TYPE_SHIFT_IMM},           // LSL Rd, Rm, #imm
    {"LSR", 0xE1A00000, TYPE_SHIFT_IMM},           // LSR Rd, Rm, #imm
    {"MUL", 0xE0000090, TYPE_MUL},                 // MUL Rd, Rm, Rs
    {"BGE", 0xDA000000, TYPE_BRANCH},              // BGE target
    {"BLT", 0xBA000000, TYPE_BRANCH},              // BLT target
    {"B",   0xEA000000, TYPE_BRANCH},              // B target
    {"SWI", 0xEF000000, TYPE_SWI},                 // SWI #imm
    {NULL, 0, 0}                                   // Sentinel
};

// Symbol table for labels
typedef struct {
  char *label;
  int address;
} Symbol;

#define MAX_SYMBOLS 100
Symbol symbol_table[MAX_SYMBOLS];
int symbol_count = 0;

// Function to add a symbol to the symbol table
void add_symbol(char *label, int address)
{
  if (symbol_count < MAX_SYMBOLS)
  {
    symbol_table[symbol_count].label = strdup(label);
    symbol_table[symbol_count].address = address;
    symbol_count++;
  }
}

// Function to get address of a symbol
int get_symbol_address(char *label)
{
  for (int i = 0; i < symbol_count; i++)
  {
    if (strcmp(symbol_table[i].label, label) == 0)
    {
      return symbol_table[i].address;
    }
  }
  return -1; // Not found
}

// Function to get register number from string (e.g., R0 -> 0)
int get_register_num(char *reg_str) {
  if (reg_str != NULL && (reg_str[0] == 'R' || reg_str[0] == 'r'))
  {
    return atoi(reg_str + 1);
  }
  return -1; // Invalid register
}

// Function to parse immediate value (handles # prefix)
int parse_immediate(char *imm_str) {
  if (imm_str != NULL && imm_str[0] == '#') {
    return atoi(imm_str + 1);
  }
  return -1; // Not an immediate value with #
}

// Function to check if a string is a register (R0-R15)
bool is_register(char *str)
{
  if (str == NULL || strlen(str) < 2 || (str[0] != 'R' && str[0] != 'r'))
  {
    return false;
  }
  for (int i = 1; i < (int)strlen(str); i++)
  {
    if (!isdigit(str[i])) {
      return false;
    }
  }
  int reg_num = atoi(str + 1);
  return reg_num >= 0 && reg_num <= 15; // ARM typically has R0-R15
}

// Function to remove comments and trim whitespace
void preprocess_line(char *line)
{
  char *comment_start = strchr(line, '@');
  if (comment_start != NULL)
  {
    *comment_start = '\0'; // Null-terminate at comment start
  }
  // Trim trailing whitespace
  int len = strlen(line);
  while (len > 0 && isspace(line[len - 1]))
  {
    line[--len] = '\0';
  }
}

int main(int argc, char *argv[])
{
  argc = 3;
  argv[1] = "AssemplyFile.text";
  argv[2] = "MachineCodeFile.text";
  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <input_file.text> <output_file.text>\n", argv[0]);
    return 1;
  }

  FILE *inputFile = fopen(argv[1], "r");
  if (inputFile == NULL)
  {
    perror("Error opening input file");
    return 1;
  }

  // First pass: Build symbol table
  char line[256];
  int current_address = 0;
  while (fgets(line, sizeof(line), inputFile) != NULL)
  {
    preprocess_line(line);
    if (strlen(line) == 0) continue; // Skip empty lines

    char *temp_line = strdup(line);
    char *token = strtok(temp_line, " \t");

    if (token != NULL)
    {
      // Check for label (ends with ':')
      if (token[strlen(token) - 1] == ':')
      {
        token[strlen(token) - 1] = '\0'; // Remove ':'
        add_symbol(token, current_address);
      }
      else
      {
        // Assume it's an instruction, increment address
        current_address += 4; // ARM instructions are 4 bytes
      }
    }
    free(temp_line);
  }
  rewind(inputFile); // Reset file pointer for second pass

  FILE *outputFile = fopen(argv[2], "w");
  if (outputFile == NULL)
  {
    perror("Error opening output file");
    fclose(inputFile);
    return 1;
  }

  // Second pass: Generate machine code
  current_address = 0;
  while (fgets(line, sizeof(line), inputFile) != NULL)
  {
    preprocess_line(line);
    if (strlen(line) == 0) continue; // Skip empty lines

    char *original_line = strdup(line);
    char *token = strtok(original_line, " \t"); // Use original_line for initial tokenizing

    if (token == NULL)
    {
      free(original_line); continue;
    }

    // If it's a label, skip it (already processed in first pass)
    if (token[strlen(token) - 1] == ':')
    {
      free(original_line);
      continue;
    }

    // It's an instruction
    char *mnemonic = token;
    unsigned int machine_code = 0;
    int instruction_found = 0;

    // Parse all arguments into separate strings first
    char *arg_tokens[4]; // Max 3 arguments + mnemonic
    int arg_count = 0;
    char *temp_line_for_args = strdup(line); // Use a fresh copy for parsing arguments
    char *arg_token = strtok(temp_line_for_args, " \t,"); // Tokenize by space, tab, or comma
    arg_token = strtok(NULL, " \t,"); // Skip mnemonic

    while (arg_token != NULL && arg_count < 4)
    {
      arg_tokens[arg_count++] = strdup(arg_token);
      arg_token = strtok(NULL, " \t,");
    }
    free(temp_line_for_args);

    for (int i = 0; instructions[i].mnemonic != NULL; i++)
    {
      if (strcmp(instructions[i].mnemonic, mnemonic) == 0)
      {
        // Handle MOV specifically as it has two distinct opcodes based on operand type
        if (strcmp(mnemonic, "MOV") == 0)
        {
          if (arg_count >= 2 && parse_immediate(arg_tokens[1]) != -1)
          { // MOV Rd, #imm
            if (instructions[i].type == TYPE_MOV_IMM)
            {
              instruction_found = 1;
              machine_code = instructions[i].opcode_base;
              int rd = get_register_num(arg_tokens[0]);
              int imm = parse_immediate(arg_tokens[1]);
              machine_code |= (rd << 12) | (imm & 0xFF); // Simplified immediate encoding
              break;
            }
          } else if (arg_count >= 2 && is_register(arg_tokens[1]))
          { // MOV Rd, Rm
            if (instructions[i].type == TYPE_MOV_REG)
            {
              instruction_found = 1;
              machine_code = instructions[i].opcode_base;
              int rd = get_register_num(arg_tokens[0]);
              int rm = get_register_num(arg_tokens[1]);
              machine_code |= (rd << 12) | (rm & 0xF); // Simplified encoding
              break;
            }
          }
          continue; // Continue to next instruction entry if type doesn't match
        }

        // For other instructions (ADD, SUB, AND, ORR, EOR, CMP)
        // that have both immediate and register forms, check the type.
        if (instructions[i].type == TYPE_DATA_PROCESSING_REG)
        { // Rd, Rn, Rm
          if (arg_count >= 3 && is_register(arg_tokens[2]))
          {
            instruction_found = 1;
            machine_code = instructions[i].opcode_base;
            int rd = get_register_num(arg_tokens[0]);
            int rn = get_register_num(arg_tokens[1]);
            int rm = get_register_num(arg_tokens[2]);
            machine_code |= (rn << 16) | (rd << 12) | (rm & 0xF); // Simplified encoding
            break;
          }
        } else if (instructions[i].type == TYPE_DATA_PROCESSING_IMM)
        { // Rd, Rn, #imm
          if (arg_count >= 3 && parse_immediate(arg_tokens[2]) != -1)
          {
            instruction_found = 1;
            machine_code = instructions[i].opcode_base;
            int rd = get_register_num(arg_tokens[0]);
            int rn = get_register_num(arg_tokens[1]);
            int imm = parse_immediate(arg_tokens[2]);
            machine_code |= (rn << 16) | (rd << 12) | (imm & 0xFF); // Simplified encoding
            break;
          }
        } else if (instructions[i].type == TYPE_CMP_REG)
        { // Rn, Rm
          if (arg_count >= 2 && is_register(arg_tokens[1]))
          {
            instruction_found = 1;
            machine_code = instructions[i].opcode_base;
            int rn = get_register_num(arg_tokens[0]);
            int rm = get_register_num(arg_tokens[1]);
            machine_code |= (rn << 16) | (rm & 0xF); // Simplified encoding
            break;
          }
        }
        else if (instructions[i].type == TYPE_CMP_IMM)
        { // Rn, #imm
          if (arg_count >= 2 && parse_immediate(arg_tokens[1]) != -1)
          {
            instruction_found = 1;
            machine_code = instructions[i].opcode_base;
            int rn = get_register_num(arg_tokens[0]);
            int imm = parse_immediate(arg_tokens[1]);
            machine_code |= (rn << 16) | (imm & 0xFF); // Simplified encoding
            break;
          }
        } else if (instructions[i].type == TYPE_MUL)
        { // MUL Rd, Rm, Rs
          if (arg_count >= 3)
          {
            instruction_found = 1;
            machine_code = instructions[i].opcode_base;
            int rd = get_register_num(arg_tokens[0]);
            int rm = get_register_num(arg_tokens[1]);
            int rs = get_register_num(arg_tokens[2]);
            machine_code |= (rm & 0xF) | ((rs & 0xF) << 8) | ((rd & 0xF) << 16); // Simplified MUL encoding
            break;
          }
        } else if (instructions[i].type == TYPE_BRANCH)
        { // BGE, BLT, B
          if (arg_count >= 1)
          {
            instruction_found = 1;
            machine_code = instructions[i].opcode_base;
            int target_address = get_symbol_address(arg_tokens[0]);
            if (target_address != -1)
            {
              machine_code |= ((target_address - current_address - 8) >> 2) & 0x00FFFFFF; // Simplified relative branch
            }
            else
            {
              fprintf(stderr, "Error: Undefined label '%s'\n", arg_tokens[0]);
            }
            break;
          }
        }
        else if (instructions[i].type == TYPE_SWI)
        { // SWI
          if (arg_count >= 1 && parse_immediate(arg_tokens[0]) != -1)
          {
            instruction_found = 1;
            machine_code = instructions[i].opcode_base;
            int imm_val = parse_immediate(arg_tokens[0]);
            machine_code |= (imm_val & 0x00FFFFFF); // Simplified SWI immediate
            break;
          }
        }
        else if (instructions[i].type == TYPE_SHIFT_IMM)
        { // LSL/LSR Rd, Rm, #imm
          if (arg_count >= 3 && parse_immediate(arg_tokens[2]) != -1)
          {
            instruction_found = 1;
            machine_code = instructions[i].opcode_base;
            int rd = get_register_num(arg_tokens[0]);
            int rm = get_register_num(arg_tokens[1]);
            int imm = parse_immediate(arg_tokens[2]);

            if (imm != -1)
            {
              machine_code |= (rm & 0xF) | ((rd & 0xF) << 12) | ((imm & 0x1F) << 7);
              if (strcmp(mnemonic, "LSR") == 0)
              {
                machine_code |= (1 << 5); // Set bit 5 for LSR
              }
            } else
            {
              fprintf(stderr, "Error: Invalid immediate operand for %s: %s\n", mnemonic, arg_tokens[2]);
            }
            break;
          }
        }
      }
    }

    if (instruction_found)
    {
      fprintf(outputFile, "%08X\n", machine_code);
      current_address += 4;
    } else {
      fprintf(stderr, "Unknown instruction or syntax error: %s\n", original_line);
    }
    free(original_line);
    for (int k = 0; k < arg_count; k++)
    {
      free(arg_tokens[k]);
    }
  }

  fclose(inputFile);
  fclose(outputFile);

  printf("ARM Assembly file assembled successfully!\n");

  return 0;
}
