'use strict';

///////////////////////////////////////////////////////// PARSING



var charIsInRange = function (arg) {
    var lower = arg[0];
    var c = arg[1]; // falsy if eof
    var upper = arg[2];
    return c &&
           lower.charCodeAt(0) <= c.charCodeAt(0) &&
           c.charCodeAt(0) <= upper.charCodeAt(0);
};

var isDigit = function (c) {
    return charIsInRange(['0', c, '9']);
};
var isLowercaseLetter = function (c) {
    return charIsInRange(['a', c, 'z']);
};
var isUppercaseLetter = function (c) {
    return charIsInRange(['A', c, 'Z']);
};
var isLetter = function (c) {
    return isLowercaseLetter(c) || isUppercaseLetter(c);
};
var whitespaces = [' ', '\n', '\r', '\t'];
var isWhitespace = function (c) {
    return whitespaces.indexOf(c) !== -1;
};

var keywords = [
    'else', 'false', 'function', 'if', 'in', 'null', 'return', 'true',
    'typeof', 'var', 'while'
];

var getLineNumberAtIndex = function (arg) {
    var lines = 1;
    var i = 0;
    while (i < arg.source.length && i < arg.index) {
        if (arg.source[i] === '\n') {
            lines = lines + 1;
        }
        i = i + 1;
    }
    return lines;
};

// Returns a list of statements (they are AST nodes).
var parse = function (source) {
    var index = 0; // Don't set directly. Use setIndex().
    var farthestIndex = 0; // Only used for error management.

    // Most of the following parsing functions return a falsy value on
    // failure, or the read substring on success.

    var setIndex = function (newIndex) {
        index = newIndex;
        if (index > farthestIndex) {
            farthestIndex = index;
        }
    };

    // Tries to read the given string.
    var read = function (expected) {
        // TODO: String#slice() is slow and performance matters here.
        if (source.slice(index).indexOf(expected) === 0) {
            setIndex(index + expected.length);
            return expected;
        }
    };

    // Tries to read one character that matches the given predicate.
    var readCharIf = function (predicate) {
        var c = source[index];
        if (predicate(c)) {
            setIndex(index + 1);
            return c;
        }
    };

    // Pretty obvious. See how it is used below.
    var backtrack = function (func) {
        return function (arg) {
            var begin = index;
            var result = func(arg);
            if (result) {
                return result;
            }
            index = begin;
        };
    };

    var createToken = function (token) {
        token.index = index;
        return token;
    };

    var digit = function () {
        return readCharIf(isDigit);
    };
    var letter = function () {
        return readCharIf(isLetter);
    };
    var whitespace = function () {
        // That's bit hackish for performance reasons.
        while (1) {
            if (source[index] === '/' && source[index + 1] === '/') {
                setIndex(index + 2);
                while (readCharIf(function (c) {return c !== '\n';})) {}
            }
            if (!readCharIf(isWhitespace)) {
                return 1;
            }
        }
    };

    var stringEscapeTable = {
        n: '\n', t: '\t', r: '\r', '\'': '\'', '\\': '\\',
    };
    var string = backtrack(function () {
        whitespace();
        var quote = read('\'');
        if (!quote) {
            return;
        }

        var value = '';
        while (1) {
            var c = source[index];
            setIndex(index + 1);
            if (!c) {
                return;
            }
            if (c === '\'') {
                return createToken({type: 'string', value});
            }
            if (c === '\\') {
                var d = source[index];
                setIndex(index + 1);
                if (!(d in stringEscapeTable)) {
                    return;
                }
                value = value + stringEscapeTable[d];
            }
            if (c !== '\\') {
                value = value + c;
            }
        }
    });

    var identifierish = backtrack(function () {
        whitespace();
        var string = letter() || read('_');
        if (!string) {
            return;
        }
        while (1) {
            var c = digit() || letter() || read('_');
            if (!c) {
                return string;
            }
            string = string + c;
        }
    });

    var identifier = backtrack(function () {
        var string = identifierish();
        if (string && keywords.indexOf(string) === -1) {
            return createToken({type: 'identifier', string});
        }
    });

    var number = backtrack(function () {
        whitespace();
        var string = digit();
        if (!string) {
            return;
        }
        while (1) {
            var c = digit();
            if (!c) {
                return createToken({
                    type: 'number',
                    value: parseInt(string)
                });
            }
            string = string + c;
        }
    });

    var keyword = backtrack(function () {
        var string = identifierish();
        if (string && keywords.indexOf(string) !== -1) {
            return string;
        }
    });

    var thisKeyword = backtrack(function (expected) {
        var string = keyword();
        if (string === expected) {
            return string;
        }
    });

    var nullExpr = function () {
        return thisKeyword('null') && {type: 'null'};
    };

    var literal = function () {
        return nullExpr() || identifier() || number() || string();
    };

    // Used to parse parentheses, braces or square brackets.
    var wrapped = backtrack(function (arg) {
        whitespace();
        if (!read(arg.left)) {
            return;
        }
        var e = arg.body();
        whitespace();
        if (read(arg.right)) {
            return e;
        }
    });

    // Used to parse lists delimited by commas or semicolons.
    var sequence = backtrack(function (arg) {
        var items = [];
        while (1) {
            if (items.length && !arg.sep()) {
                return items;
            }
            var item = arg.item();
            if (!item) {
                return items;
            }
            items.push(item);
        }
    });

    var paren = function () {
        var body = function () {
            return expr() || {type: 'null'};
        };
        return wrapped({left: '(', body, right: ')'});
    };

    var statements = function () {
        return sequence({item: statement, sep: whitespace});
    };

    var block = function () {
        return wrapped({left: '{', body: statements, right: '}'});
    };

    var commaSeparated = function (item) {
        return sequence({
            item,
            sep: function () {return read(',');}
        });
    };

    var list = function () {
        var children = wrapped({
            left: '[',
            body: function () {return commaSeparated(expr);},
            right: ']'
        });
        if (children) {
            return {type: 'list', children};
        }
    };

    var dictEntry = backtrack(function () {
        var left = identifier() || string();
        if (!left) {
            return;
        }
        left.type = 'string';
        left.value = left.string || left.value;
        whitespace();
        if (!read(':')) {
            var right = {
                type: 'identifier',
                string: left.value
            };
            return {type: 'dictEntry', left, right};
        }
        var right = expr();
        if (right) {
            return {type: 'dictEntry', left, right};
        }
    });

    var dict = function () {
        var children = wrapped({
            left: '{',
            body: function () {return commaSeparated(dictEntry);},
            right: '}'
        });
        if (children) {
            return {type: 'dict', children};
        }
    };

    var atom = function () {
        return literal() || list() || dict() || paren();
    };

    var postfixDot = backtrack(function () {
        whitespace();
        return read('.') && identifier();
    });

    var postfixRight = function (left) {
        var right = paren();
        if (right) {
            return {type: 'binaryOp', left, op: 'call', right};
        }
        if (right = wrapped({left: '[', body: expr, right: ']'})) {
            return {type: 'subscript', left, right};
        }
        if (right = postfixDot()) {
            return {
                type: 'subscript',
                left,
                right: {type: 'string', value: right.string},
            };
        }
        return;
    };

    var postfix = function () {
        var left = atom();
        while (left) {
            var n = postfixRight(left);
            if (!n) {
                return left;
            }
            left = n;
        }
        return left;
    };

    var whileStatement = backtrack(function () {
        if (!thisKeyword('while')) {
            return;
        }
        var cond = paren();
        var b = block();
        if (cond && b) {
            return {type: 'while', cond, children: b};
        }
    });

    var ifStatement = backtrack(function () {
        if (!thisKeyword('if')) {
            return;
        }
        var cond = paren();
        var b = block();
        if (cond && b) {
            return {type: 'if', cond, children: b};
        }
    });

    var varExpr = backtrack(function () {
        if (!thisKeyword('var')) {
            return;
        }
        var name = identifier();
        whitespace();
        var eq = read('=');
        var value = expr();
        if (name && eq && value) {
            return {type: 'var', name, value};
        }
    });

    var returnExpr = backtrack(function () {
        if (!thisKeyword('return')) {
            return;
        }
        return {
            type: 'return',
            value: expr() || {type: 'null'}
        };
    });

    var func = backtrack(function () {
        if (!thisKeyword('function')) {
            return;
        }
        var optionalId = function () {
            return identifier() || {type: 'null'};
        };
        var param = wrapped({left: '(', body: optionalId, right: ')'});
        var b = block();
        if (param && b) {
            return {type: 'function', param, children: b};
        }
    });

    var binaryOp = function (arg) {
        return backtrack(function () {
            var left = arg.next();
            if (!left) {
                return;
            }
            while (1) {
                var op = arg.parseOp();
                if (!op) {
                    return left;
                }
                var right = arg.next();
                left = {type: 'binaryOp', left, op, right};
            }
            return left;
        });
    };

    var binaryOp2 = function (opStrings) {
        return function (next) {
            var parseOp = function () {
                var i = 0;
                whitespace();
                while (i < opStrings.length) {
                    var op = read(opStrings[i]);
                    if (op) {
                        return op;
                    }
                    i = i + 1;
                }
            };

            return binaryOp({next, parseOp});
        };
    };

    var unary = backtrack(function () {
        whitespace();
        var op = read('-') || read('!') || thisKeyword('typeof');
        var right = postfix();
        if (op && right) {
            return {type: 'unaryOp', op, right};
        }
        return right;
    });

    var multiplication = binaryOp2(['*', '/', '%'])(unary);
    var addition = binaryOp2(['+', '-'])(multiplication);
    var relation = binaryOp2(['<=', '>=', '<', '>', 'in'])(addition);
    var equality = binaryOp2(['===', '!=='])(relation);
    var andExpr = binaryOp2(['&&'])(equality);
    var orExpr = binaryOp2(['||'])(andExpr);

    var assignment_ = backtrack(function () {
        var left = postfix();
        whitespace();
        if (!left || !read('=')) {
            return;
        }
        var right = assignment();
        if (right) {
            return {type: 'assignment', left, right};
        }
    });
    var assignment = function () {
        return assignment_() || orExpr() || func();
    };

    var expr = function () {
        return func() || varExpr() || returnExpr() || assignment();
    };

    var exprStatement = backtrack(function () {
        var e = expr();
        whitespace();
        if (read(';')) {
            return e;
        }
    });

    var statement = function () {
        return whileStatement() || ifStatement() || exprStatement();
    };

    var result = statements();
    whitespace();
    if (!result || index < source.length) {
        if (index > farthestIndex) {
            farthestIndex = index;
        }
        var line = getLineNumberAtIndex({source, index: farthestIndex});
        die('syntax error (line ' + line + ')');
    }
    return result;
};



//////////////////////////////////////////////////// BYTECODE GENERATION



var opSignsToNames = {
    '+': 'add', '-': 'sub', '*': 'mul', '/': 'div', '%': 'mod',
    '===': 'eq', '!==': 'neq',
    '<': 'lt', '>': 'gt', '<=': 'lte', '>=': 'gte',
    'in': 'in', call: 'call',
};

var unarySignsToNames = {
    '-': 'unary_minus',
    '!': 'not',
    'typeof': 'typeof'
};

var compileFunctionBody = function (statements) {
    var code = [];
    var consts = [];

    var toUint16 = function (n) {
        return [Math.floor(n / 256), n % 256]; // big endian
    };

    var genUint16 = function (n) {
        code = code.concat(toUint16(n));
    };

    var setUint16At = function (arg) {
        var number = arg[0];
        var index = arg[1];
        code[index] = toUint16(number)[0];
        code[index + 1] = toUint16(number)[1];
    };

    var genLoadConst = function (c) {
        code.push('load_const');
        genUint16(consts.length);
        consts.push(c);
    };

    var compileAssign = function (expr) {
        if (expr.left.type === 'identifier') {
            compileExpr(expr.right);
            code.push('dup');
            genLoadConst(expr.left.string);
            code.push('rot');
            code.push('store_var');
            return;
        }
        if (expr.left.type === 'subscript') {
            compileExpr(expr.right);
            code.push('dup');
            compileExpr(expr.left.left);
            compileExpr(expr.left.right);
            code.push('set');
            return;
        }
        die('unknown lvalue type');
    };

    var compileAndOr = function (expr) {
        // Shortcuts right operand. We need a `goto`. A plain old binary
        // operator is not suitable.
        compileExpr(expr.left);
        code.push('dup');
        if (expr.op === '&&') {
            code.push('not');
        }
        code.push('goto_if');
        var breakLabel = code.length;
        genUint16(0);
        code.push('pop');
        compileExpr(expr.right);
        setUint16At([code.length, breakLabel]);
    };

    var compileExpr = function (expr) {
        if (expr.type === 'string' || expr.type === 'number') {
            return genLoadConst(expr.value);
        }

        if (expr.type === 'null') {
            return code.push('load_null');
        }

        if (expr.type === 'assignment') {
            return compileAssign(expr);
        }

        if (expr.type === 'binaryOp') {
            if (expr.op === '&&' || expr.op === '||') {
                return compileAndOr(expr);
            }

            compileExpr(expr.left);
            compileExpr(expr.right);
            if (!opSignsToNames[expr.op]) {
                die('unknown op ' + expr.op);
            }
            code.push(opSignsToNames[expr.op]);
            return;
        }

        if (expr.type === 'unaryOp') {
            compileExpr(expr.right);
            code.push(unarySignsToNames[expr.op]);
            return;
        }

        if (expr.type === 'identifier') {
            genLoadConst(expr.string);
            code.push('load_var');
            return;
        }

        if (expr.type === 'function') {
            code.push('load_func');
            genUint16(expr._id);
            return;
        }

        if (expr.type === 'subscript') {
            compileExpr(expr.left);
            compileExpr(expr.right);
            code.push('get');
            return;
        }

        if (expr.type === 'list') {
            code.push('load_empty_list');
            var i = 0;
            while (i < expr.children.length) {
                compileExpr(expr.children[i]);
                code.push('list_push');
                i = i + 1;
            }
            return;
        }

        if (expr.type === 'dict') {
            code.push('load_empty_dict');
            var i = 0;
            while (i < expr.children.length) {
                var entry = expr.children[i];
                compileExpr(entry.left);
                compileExpr(entry.right);
                code.push('dict_push');
                i = i + 1;
            }
            return;
        }
        die('unknown expr type');
    };

    var compileStatement = function (expr) {
        if (expr.type === 'var') {
            genLoadConst(expr.name.string);
            code.push('dup');
            code.push('decl_var');
            compileExpr(expr.value);
            code.push('store_var');
            return;
        }

        if (expr.type === 'return') {
            compileExpr(expr.value);
            code.push('return');
            return;
        }

        if (expr.type === 'while') {
            // This is what people call "spaghetty code".
            var beginLabel = code.length;
            compileExpr(expr.cond);
            code.push('not');
            code.push('goto_if');
            var breakLabel = code.length;
            genUint16(0);
            compileStatements(expr.children);
            code.push('goto');
            genUint16(beginLabel);
            setUint16At([code.length, breakLabel]);
            return;
        }

        if (expr.type === 'if') {
            compileExpr(expr.cond);
            code.push('not');
            code.push('goto_if');
            var breakLabel = code.length;
            genUint16(0);
            compileStatements(expr.children);
            setUint16At([code.length, breakLabel]);
            return;
        }

        compileExpr(expr);
        code.push('pop'); // In statements like `print("hello");`, we
        // don't care about the value returned by `print`.
    };

    var compileStatements = function (statements) {
        var i = 0;
        while (i < statements.length) {
            compileStatement(statements[i]);
            i = i + 1;
        }
    };

    compileStatements(statements);
    code.push('load_null');
    code.push('return');

    return {consts, code};
};

// Returns a list of the functions present in the given AST node.
// Performs a depth-first search.
var getFunctions = function (node) {
    if (node.type === 'var') {
        return getFunctions(node.value);
    }

    if (node.type === 'function') {
        return [node].concat(getFunctionsInList(node.children));
    }

    if (node.type === 'while' || node.type === 'if') {
        return getFunctionsInList(node.children)
            .concat(getFunctions(node.cond));
    }

    if (['binaryOp', 'assignment', 'subscript'].indexOf(node.type) !== -1) {
        return getFunctions(node.left)
            .concat(getFunctions(node.right));
    }

    if (node.type === 'unaryOp') {
        return getFunctions(node.right);
    }

    if (node.type === 'return') {
        return getFunctions(node.value);
    }

    if (node.type === 'list' || node.type === 'dict') {
        return getFunctionsInList(node.children);
    }

    if (node.type === 'dictEntry') {
        return getFunctions(node.right);
    }

    return [];
};

// Exactly like `getFunctions` but works on a list of AST nodes.
var getFunctionsInList = function (list) {
    var funcs = [];
    var i = 0;
    while (i < list.length) {
        funcs = funcs.concat(getFunctions(list[i]));
        i = i + 1;
    }
    return funcs;
};

// Returns a list of compiled functions.
var codegen = function (statements) {
    var root = {
        type: 'function',
        children: statements,
        param: {type: 'null'}
    };

    // Assign an unique `_id` property to each function. The first function
    // must be the entrypoint.
    var functions = getFunctions(root);
    var i = 0;
    while (i < functions.length) {
        functions[i]._id = i;
        i = i + 1;
    }

    var compiledFuncs = [];
    i = 0;
    while (i < functions.length) {
        var func = functions[i];
        var compiled = compileFunctionBody(func.children);
        if (func.param.type !== 'null') {
            compiled.paramName = func.param.string;
        }
        compiledFuncs.push(compiled);
        i = i + 1;
    }
    return compiledFuncs;
};


module.exports = function (source) {
    return codegen(parse(source));
};
