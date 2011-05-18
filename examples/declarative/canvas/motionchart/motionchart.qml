import QtQuick 2.0

Canvas {
  id:motionChart
  width:1300
  height: 700
  property int progress:0
  property variant applesFrom: [1000, 300];
  property variant applesTo:[1200, 400];
  property variant orangesFrom: [1150, 200];
  property variant orangesTo:[250,  550];
  property variant bananasFrom: [300, 250];
  property variant bananasTo:[788, 617];

  property date startDate:new Date (1988,0,1)
  property date endDate:new Date (1989,6,1)
  property variant title:["Fruit", "Sales", "Expenses", "Location"];
  property bool clearTrace:true
  Text {id:appleText; text:"Apples"; font.bold:true; font.pixelSize:12; opacity:0}
  Text {id:orangeText; text:"Oranges"; font.bold:true; font.pixelSize:12; opacity:0}
  Text {id:bananaText; text:"Bananas"; font.bold:true; font.pixelSize:12; opacity:0}

  Text {id:sales; text: "700 Sales"; x:15; y:15;font.bold:true; font.pixelSize:15; opacity:0}
  Text {id:expenses; text: "Expenses 1300"; x:1170; y:670;font.bold:true; font.pixelSize:15; opacity:0}
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
      setup();
      timer.running = true;
      motionChart.clearTrace = true;
    }
    onDoubleClicked : {
      motionChart.progress = 0;
      setup();
      timer.running = true;
      motionChart.clearTrace = false;
    }
  }

  function setup() {
    var ctx = motionChart.getContext("2d");
    ctx.globalCompositeOperation = "source-over";
    ctx.clearRect(0, 0, motionChart.width, motionChart.height);

    ctx.strokeColor = Qt.rgba(133, 133, 133,1);
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(10,690);
    ctx.lineTo(10, 5);
    ctx.moveTo(10,690);
    ctx.lineTo(1295, 690);
    
    for ( var i = 0; i < 10; i++) {
      ctx.moveTo(10, i*70);
      ctx.lineTo(15, i*70);
      ctx.moveTo(i*130, 690);
      ctx.lineTo(i*130, 685);
    }
    
    ctx.stroke();
    sales.opacity =1;
    expenses.opacity = 1;
    appleText.opacity  = 1;
    orangeText.opacity  = 1;
    bananaText.opacity = 1;
  }

  function draw() {
    var totalDays = Math.ceil((endDate.getTime()-startDate.getTime())/(1000*60*60*24));
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

    if (motionChart.clearTrace)
       ctx.clearRect(15, 15, motionChart.width - 15, motionChart.height - 30);

     
    //apples
    ctx.fillColor = Qt.rgba(0,255,0,1);

    ctx.beginPath();
    ctx.arc( apples[0] , 700 - apples[1] , 20 , 0 , Math.PI * 2 , true );
    //ctx.closePath();
    ctx.fill();        
    ctx.fillRect(apples[0], 700 - apples[1], 28, 28);
    appleText.x = apples[0];
    appleText.y = 700 - apples[1]  - 30;
    //oranges
    ctx.fillColor = Qt.rgba(0,0,255,1);
    ctx.beginPath();
    ctx.arc( oranges[0], 700 - oranges[1] , 20 , 0 , Math.PI * 2 , true );
    ctx.closePath();
    ctx.fill();        
    ctx.fillRect(oranges[0], 700 - oranges[1], 28, 28);
    orangeText.x = oranges[0];
    orangeText.y = 700 - oranges[1]  - 30;

    //bananas
    ctx.fillColor = Qt.rgba(255,0,0,1);
    ctx.beginPath();
    ctx.arc( bananas[0] , 700 - bananas[1] , 20 , 0 , Math.PI * 2 , true );
    ctx.closePath();
    ctx.fill();        
    ctx.fillRect(bananas[0], 700 - bananas[1], 28, 28);
    bananaText.x = bananas[0];
    bananaText.y = 700 - bananas[1]  - 30;

    ctx.sync();
    motionChart.progress ++; 
  }
}
