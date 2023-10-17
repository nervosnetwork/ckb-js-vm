"use strict";

function assert(actual, expected, message) {
    if (arguments.length == 1)
        expected = true;

    if (actual == expected)
        return;

    throw Error("assertion failed: got |" + actual + "|" +
                ", expected |" + expected + "|" +
                (message ? " (" + message + ")" : ""));
}

function test_to_string() {
    assert(1.1.toString(), "1.1")
    assert(-1.1.toString(), "-1.100000")
    assert(3.14159265354.toString(), "3.1415926540000000")
}

test_to_string()
