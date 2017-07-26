
// Used for boostrapping.

require('./util.node.js')
var fs = require('fs')
var translate = require('./translate_to_c')

console.log(translate(fs.readFileSync('compile.js').toString()))
