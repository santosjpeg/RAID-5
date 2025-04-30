#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Converts hexadecimal string to byte array
void hex_to_bytes(const char *hex, uint8_t *bytes, size_t len) {
  for (size_t i = 0; i < len; i++) {
    sscanf(hex + 2 * i, "%2hhx", &bytes[i]);
  }
}

// Converts byte array to a he xadecimal string
void bytes_to_hex(const uint8_t *bytes, size_t len, char *hex) {
  for (size_t i = 0; i < len; i++) {
    sprintf(hex + 2 * i, "%02x", bytes[i]);
  }
}

int main(int argc, char *argv[]) {
  // Validate proper usage
  if (argc < 5) {
    fprintf(stderr, "USAGE: <block_size> <bytes_stored> <disk_path> "
                    "<disk_bytes> ...");
    return 1;
  }

  // Parse arguments
  int B = atoi(argv[1]);
  int J = atoi(argv[2]);
  char *reg_disk_path = argv[3];
  int K = atoi(argv[4]);
  int N = argc - 5;

  if (N < 3) {
    fprintf(stderr, "RAID5 requires at least 3 disks\n");
    return 1;
  }

  // Check K is multiple of B
  if (K % B != 0) {
    fprintf(stderr, "K must be a multiple of B\n");
    return 1;
  }

  int S = K / B; // Number of stripes
  int total_data_blocks_needed = S * (N - 1);

  // Read regular disk data
  FILE *reg_file = fopen(reg_disk_path, "r");
  if (!reg_file) {
    perror("Failed to open regular disk file");
    return 1;
  }

  // Read entire file content
  fseek(reg_file, 0, SEEK_END);
  long file_size = ftell(reg_file);
  fseek(reg_file, 0, SEEK_SET);

  char *reg_hex = malloc(2 * J + 1);
  if (!reg_hex) {
    perror("malloc failed");
    fclose(reg_file);
    return 1;
  }

  size_t bytes_read = fread(reg_hex, 1, 2 * J, reg_file);
  fclose(reg_file);

  if (bytes_read != 2 * J) {
    fprintf(stderr, "Regular disk file size does not match J\n");
    free(reg_hex);
    return 1;
  }

  reg_hex[2 * J] = '\0';

  // Convert to bytes
  uint8_t *reg_data = malloc(J);
  hex_to_bytes(reg_hex, reg_data, J);
  free(reg_hex);

  // Prepare data blocks
  int num_original_blocks = J / B;
  int total_data_bytes = total_data_blocks_needed * B;
  uint8_t *data_blocks = calloc(total_data_bytes, 1);
  memcpy(data_blocks, reg_data, J);
  free(reg_data);

  // Initialize RAID disk buffers
  uint8_t **disk_buffers = malloc(N * sizeof(uint8_t *));
  for (int i = 0; i < N; i++) {
    disk_buffers[i] = malloc(K);
    memset(disk_buffers[i], 0, K);
  }

  // Process each stripe
  for (int s = 0; s < S; s++) {
    int p = (N - 1) - (s % N);

    // Determine non-parity disks in order
    int non_parity_order[N - 1];
    int current = (p + 1) % N;
    int count = 0;
    while (count < N - 1) {
      if (current != p) {
        non_parity_order[count] = current;
        count++;
      }
      current = (current + 1) % N;
    }

    // Compute parity block
    uint8_t parity_block[B];
    memset(parity_block, 0, B);
    for (int i = 0; i < N - 1; i++) {
      int block_idx = s * (N - 1) + i;
      uint8_t *block = data_blocks + block_idx * B;
      for (int j = 0; j < B; j++) {
        parity_block[j] ^= block[j];
      }
    }

    // Assign data blocks and parity
    for (int d = 0; d < N; d++) {
      if (d == p) {
        memcpy(disk_buffers[d] + s * B, parity_block, B);
      } else {
        // Find position in non_parity_order
        int pos = -1;
        for (int i = 0; i < N - 1; i++) {
          if (non_parity_order[i] == d) {
            pos = i;
            break;
          }
        }
        if (pos == -1) {
          fprintf(stderr, "Disk %d not in non_parity_order\n", d);
          exit(1);
        }
        int block_idx = s * (N - 1) + pos;
        uint8_t *src = data_blocks + block_idx * B;
        memcpy(disk_buffers[d] + s * B, src, B);
      }
    }
  }

  // Write RAID disk files
  for (int i = 0; i < N; i++) {
    char *hex_output = malloc(2 * K + 1);
    bytes_to_hex(disk_buffers[i], K, hex_output);
    FILE *f = fopen(argv[5 + i], "w");
    if (!f) {
      perror("Failed to open disk file");
      exit(1);
    }
    fwrite(hex_output, 1, 2 * K, f);
    fclose(f);
    free(hex_output);
    free(disk_buffers[i]);
  }

  free(disk_buffers);
  free(data_blocks);

  return 0;
}
