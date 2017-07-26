
var bottleCountString = function (bottles) {
    return function (capitalize) {
        if (bottles > 1) {
            return bottles + ' bottles';
        }
        if (bottles === 1) {
            return '1 bottle';
        }
        if (capitalize) {
            return 'No more bottles';
        }
        return 'no more bottles';
    };
};

var drink = function (bottles) {
    print(bottleCountString(bottles)(1) + ' of beer on the wall, ' +
          bottleCountString(bottles)(0) + ' of beer.');

    if (!bottles) {
        print('We\'ve taken them down and passed them around; now we\'re ' +
              'drunk and passed out!');
        return;
    }

    var cs = bottleCountString(bottles - 1)(0);
    print('Take one down, pass it around, ' + cs + ' of beer on the wall.');
    drink(bottles - 1);
};

drink(99);
