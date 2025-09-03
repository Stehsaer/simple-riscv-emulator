while True:
    input_str = input()
    checksum = sum(ord(c) for c in input_str) % 256
    print(f"${input_str}#{checksum:02x}")
