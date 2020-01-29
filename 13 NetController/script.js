console.log("Loading");

var canvas = document.getElementById("canvas");
var ctx = canvas.getContext("2d");

function clear() {
   canvas.height = canvas.clientHeight;
   canvas.width = canvas.clientWidth;
   ctx.fillStyle = "#FFFFBB";
   ctx.fillRect(0,0,canvas.width, canvas.height);
}

function setTxt(s) {
   txt = document.getElementById("txt");
   txt.innerHTML = s;
}

var drag = false;
var dragStart;
var dragEnd;

canvas.addEventListener("mousedown", function(event) {
   dragStart = {
      x: event.pageX - canvas.offsetLeft,
      y: event.pageY - canvas.offsetTop
   };
   drag = true;
   console.log("mousedown");
   
   clear();
   ctx.fillStyle = "#BBBB00";
   ctx.fillRect(dragStart.x,dragStart.y,10, 10);
});
canvas.addEventListener("mousemove", function(event) {
   if(drag) {
      dragEnd = {
         x: event.pageX - canvas.offsetLeft,
         y: event.pageY - canvas.offsetTop
      }
      var dx = (dragEnd.x - dragStart.x) / canvas.width;
      var dy = dragEnd.y - dragStart.y / canvas.height;
      console.log(dx + "" + dy);
      ctx.fillStyle = "#BB0000";
      ctx.fillRect(dragEnd.x,dragEnd.y,10, 10);
   }
});
canvas.addEventListener("mouseup", function(event) {
   drag = false;
   console.log("mouseup");
   clear();
});

function rgb(r, g, b){
   return ["rgb(",r,",",g,",",b,")"].join("");
}


var ntouches = 20;
var tdata = {};

function initTData() {
   var i;
   for(i=0; i<ntouches; i++) {
      tdata[i] = {
         drag: false,
	 x0: 0,
	 y0: 0,
	 x1: 0,
	 y1: 0,
	 color: rgb(i*30 % 256, i*50 % 256, i*70 % 256),
      }
   }
}
initTData();

canvas.addEventListener("touchstart", function(event) {
   event.preventDefault();
   var touches = event.changedTouches;
   for(var i=0; i<touches.length; i++) {
      var id = touches[i].identifier;
      
      tdata[id].drag = true;
      tdata[id].x0 = (touches[i].pageX - canvas.offsetLeft);
      tdata[id].y0 = (touches[i].pageY - canvas.offsetTop);
      tdata[id].x1 = tdata[id].x0;
      tdata[id].y1 = tdata[id].y0;
      
      ctx.fillStyle = tdata[id].color;
      ctx.fillRect(tdata[id].x0, tdata[id].y0, 50, 50);
   }
});
canvas.addEventListener("touchmove", function(event) {
   var touches = event.changedTouches;
   for(var i=0; i<touches.length; i++) {
      var id = touches[i].identifier;
      
      tdata[id].x1 = (touches[i].pageX - canvas.offsetLeft);
      tdata[id].y1 = (touches[i].pageY - canvas.offsetTop);
      
      var dx = tdata[id].x1 - tdata[id].x0;
      var dy = tdata[id].y1 - tdata[id].y0;
      var d = Math.sqrt(dx*dx + dy*dy);
      var dmax = 0.01*Math.min(canvas.height,canvas.width);
      if(d>dmax) {
         tdata[id].x0 = tdata[id].x1 - dmax*dx/d
         tdata[id].y0 = tdata[id].y1 - dmax*dy/d
      }
      
      ctx.fillStyle = tdata[id].color;
      ctx.fillRect(tdata[id].x1, tdata[id].y1, 10,10);
   }
});
canvas.addEventListener("touchend", function(event) {
   var touches = event.changedTouches;
   for(var i=0; i<touches.length; i++) {
      var id = touches[i].identifier;
      
      tdata[id].drag = false;
   }
});

var cnt = 0;
function run() {
   cnt++;
   console.log("run " + cnt);
   
   var isDown = false;
   var dx = 0;
   var dy = 0;
   if(drag) {
      dx = (dragEnd.x - dragStart.x) / canvas.width;
      dy = (dragEnd.y - dragStart.y) / canvas.height;
   } else if(tdata[0].drag) {
      dx = (tdata[0].x1 - tdata[0].x0) / canvas.width;
      dy = (tdata[0].y1 - tdata[0].y0) / canvas.height;
   }
   
   var url = "data?uid="+getCookie("userid")+"&c="+cnt+"&dx="+dx+"&dy="+dy;
   fetch(url)
   .then(data => {return data.text()})
   .then(txt => {console.log("data: "+txt)})
   .catch(error => console.log(error))
}

run()

var timer = setInterval(run, 50);

clear();



