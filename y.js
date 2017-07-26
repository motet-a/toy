
// You are not expected to understand this.
var Y = function (le) {
    return (function(f) {
        return f(f);
    })(function (f) {
        return le(function (x) {
            return (f(f))(x);
        });
    });
};

var fac = function (fac2) {
    return function (n) {
        if (n <= 1) {
            return 1;
        }
        return n * fac2(n - 1);
    };
};

print(Y(fac)(5));
