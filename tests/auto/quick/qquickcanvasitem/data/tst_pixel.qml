import QtQuick 2.0

CanvasTestCase {
   id:testCase
   name: "pixel"
   function init_data() { return testData("2d"); }
   function test_createImageData(row) {
       var canvas = createCanvasObject(row);
       var ctx = canvas.getContext('2d');
       ctx.reset();
       canvas.destroy()
  }
   function test_getImageData(row) {
       var canvas = createCanvasObject(row);
       var ctx = canvas.getContext('2d');
       ctx.reset();
       canvas.destroy()
  }
   function test_object(row) {
       var canvas = createCanvasObject(row);
       var ctx = canvas.getContext('2d');
       ctx.reset();
       canvas.destroy()
  }
   function test_putImageData(row) {
       var canvas = createCanvasObject(row);
       var ctx = canvas.getContext('2d');
       ctx.reset();
       canvas.destroy()
  }
   function test_filters(row) {
       var canvas = createCanvasObject(row);
       var ctx = canvas.getContext('2d');
       ctx.reset();
       canvas.destroy()
  }
}
