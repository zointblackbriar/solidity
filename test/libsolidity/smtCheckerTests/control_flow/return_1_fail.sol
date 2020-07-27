pragma experimental SMTChecker;

contract C {
	function add(uint x, uint y) internal pure returns (uint) {
		if (y == 0)
			return x;
		if (y == 1)
			return ++x;
		if (y == 2)
			return x + 2;
		return x + y;
	}

	function f() public pure {
		assert(add(100, 0) != 100);
		assert(add(100, 1) != 101);
		assert(add(100, 2) != 102);
		assert(add(100, 100) != 200);
	}
}
// ----
// Warning 6328: (244-270): Assertion violation happens here
// Warning 6328: (274-300): Assertion violation happens here
// Warning 6328: (304-330): Assertion violation happens here
// Warning 6328: (334-362): Assertion violation happens here
// Warning 2661: (158-161): Overflow (resulting value larger than 2**256 - 1) happens here
// Warning 2661: (187-192): Overflow (resulting value larger than 2**256 - 1) happens here
// Warning 2661: (203-208): Overflow (resulting value larger than 2**256 - 1) happens here
