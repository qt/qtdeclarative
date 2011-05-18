import QtQuick 2.0

Canvas {
  id:motionChart
  width:1300
  height: 700
  property int progress:0
  property applesFrom: [1000, 300];
  property applesTo:[1200, 400];
  property orangesFrom: [1150, 200];
  property orangesTo:[750,  150];
  property bananasFrom: [300, 250];
  property bananasTo:[788, 617];

  property date startDate:new Date (1988,0,1)
  property date endDate:new Date (1989,6,1)
  property variant title:["Fruit", "Sales", "Expenses", "Location"];

  Timer {
    id:timer
    interval: 1; running: false; repeat: true
    onTriggered: {
      motionChart.draw();
    }
  }

  MouseArea {
    anchors.fill: parent
    onPressed : {
      motionChart.progress = 0;
      timer.running = true;
    }
  }

  function draw() {
    var int totalDays = Math.ceil((endDate.getTime()-startDate.getTime())/(1000*60*60*24));
    if (motionChart.progress >= totalDays) {
      timer.running = false;
      return;
    } 
    var apples = [];
    apples[0] = applesFrom[0] + ((applesTo[0] - applesFrom[0]) * (motionChart.progress/totalDays));
    apples[1] = applesFrom[1] + ((applesTo[1] - applesFrom[1]) * (motionChart.progress/totalDays));

    var oranges = [];
    oranges[0] = orangesFrom[0] + ((orangesTo[0] - orangesFrom[0]) * (motionChart.progress/totalDays));
    oranges[1] = orangesFrom[1] + ((orangesTo[1] - orangesFrom[1]) * (motionChart.progress/totalDays));
 
    var bananas = [];
    bananas[0] = bananasFrom[0] + ((bananasTo[0] - bananasFrom[0]) * (motionChart.progress/totalDays));
    bananas[1] = bananasFrom[1] + ((bananasTo[1] - bananasFrom[1]) * (motionChart.progress/totalDays));

    var ctx = motionChart.getContext("2d");
    ctx.globalCompositeOperation = "source-over";

    ctx.clearRect(0, 0, motionChart.width, motionChart.height);
    //apples
    ctx.fillColor = Qt.rgba(0,1,0,1);
    ctx.beginPath();
    ctx.arc( apples[0] , 700 - apples[1] , 20 , 0 , Math.PI * 2 , true );
    ctx.closePath();
    ctx.fill();        

    //oranges
    ctx.fillColor = Qt.rgba(0,1,0,1);
    ctx.beginPath();
    ctx.arc( oranges[0], 700 - oranges[1] , 20 , 0 , Math.PI * 2 , true );
    ctx.closePath();
    ctx.fill();        

    //bananas
    var bananaX =;
    var bananaY =;
    ctx.fillColor = Qt.rgba(0,1,0,1);
    ctx.beginPath();
    ctx.arc( bananas[0] , 700 - bananas[1] , 20 , 0 , Math.PI * 2 , true );
    ctx.closePath();
    ctx.fill();        

    motionChart.progress ++; 
  }
}
