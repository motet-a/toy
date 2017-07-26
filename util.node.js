
// Polyfills in order to run Toy scripts with Node.js

global.die = function (message) {
    console.error(message);
    process.exit(1);
};

global.print = function (message) {
    console.log(message);
};
