contract C {
    function f(uint a, uint b) public pure returns (uint x) {
        x = a * b;
    }
    function g(uint8 a, uint8 b) public pure returns (uint8 x) {
        x = a * b;
    }
}
// ====
// compileViaYul: true
// ----
// f(uint256,uint256): 5, 6 -> 30
// f(uint256,uint256): -1, 1 -> -1
// f(uint256,uint256): -1, 2 -> FAILURE, hex"4e487b71", 0x11
// f(uint256,uint256): 0x8000000000000000000000000000000000000000000000000000000000000000, 2 -> FAILURE, hex"4e487b71", 0x11
// f(uint256,uint256): 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF, 2 -> -2
// f(uint256,uint256): 2, 0x8000000000000000000000000000000000000000000000000000000000000000 -> FAILURE, hex"4e487b71", 0x11
// f(uint256,uint256): 2, 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF -> -2
// f(uint256,uint256): 0x0100000000000000000000000000000000, 0x0100000000000000000000000000000000 -> FAILURE, hex"4e487b71", 0x11
// f(uint256,uint256): 0x0100000000000000000000000000000000, 0x00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF -> 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000000000000000000000000000
// f(uint256,uint256): 0x00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF, 0x0100000000000000000000000000000000 -> 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000000000000000000000000000
// f(uint256,uint256): 0x0100000000000000000000000000000001, 0x00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF -> -1
// f(uint256,uint256): 0x00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF, 0x0100000000000000000000000000000001 -> -1
// f(uint256,uint256): 0x0100000000000000000000000000000002, 0x00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF -> FAILURE, hex"4e487b71", 0x11
// f(uint256,uint256): 0x00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF, 0x0100000000000000000000000000000002 -> FAILURE, hex"4e487b71", 0x11
// f(uint256,uint256): -1, 0 -> 0
// f(uint256,uint256): 0, -1 -> 0
// g(uint8,uint8): 5, 6 -> 30
// g(uint8,uint8): 0x80, 2 -> FAILURE, hex"4e487b71", 0x11
// g(uint8,uint8): 0x7F, 2 -> 254
// g(uint8,uint8): 2, 0x7F -> 254
// g(uint8,uint8): 0x10, 0x10 -> FAILURE, hex"4e487b71", 0x11
// g(uint8,uint8): 0x0F, 0x11 -> 0xFF
// g(uint8,uint8): 0x0F, 0x12 -> FAILURE, hex"4e487b71", 0x11
// g(uint8,uint8): 0x12, 0x0F -> FAILURE, hex"4e487b71", 0x11
// g(uint8,uint8): 0xFF, 0 -> 0
// g(uint8,uint8): 0, 0xFF -> 0
