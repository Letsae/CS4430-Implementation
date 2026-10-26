#!/usr/bin/env python3
import hashlib
import secrets
import binascii

def generate_simple_hash(password):
    """Generate hash in the same format as BcryptPasswordHasher"""
    # Generate 16-byte salt
    salt = secrets.token_bytes(16)
    # Hash password + salt (simplified - using the algorithm from BcryptPasswordHasher.cpp)
    salted_password = salt + password.encode('utf-8')
    # Use std::hash equivalent (Python's hash is different, so we'll use SHA-256 for consistency)
    # The C++ code uses std::hash iterated 1000 times
    hash_value = int(hashlib.sha256(salted_password).hexdigest(), 16)
    for _ in range(999):
        hash_value = int(hashlib.sha256((str(hash_value) + password).encode()).hexdigest(), 16)

    # Format: $simple$<salt-hex>$<hash>
    salt_hex = binascii.hexlify(salt).decode('utf-8')
    hash_hex = format(hash_value, '016x')  # 16 hex chars like the C++ version
    return f"$simple${salt_hex}${hash_hex}"

if __name__ == "__main__":
    password = "password"
    hash_value = generate_simple_hash(password)
    print(f"Hash for '{password}': {hash_value}")
