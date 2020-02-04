console.log("Loading");

// Idea:
// string defines what elements should be on screen.
// - buttons - key?
// - 2d-knobs (steering) - keys?
// each element has an id.
// when some buttons (dis)appear: still keep knobs same (continuity).
// server keeps mapping of id-elements also -> interpred input.
// expect screen in landscape.

elementsString = "";
elements = {}
// id -> element

class Element {
   constructor(id,x0,y0,x1,y1) {
      this.id = id;
      this.x0=parseFloat(x0);
      this.y0=parseFloat(y0);
      this.x1=parseFloat(x1);
      this.y1=parseFloat(y1);
      this.canCapture = true;
      this.isCaptured = undefined;
      this.color = "#FF0000"
   }
   draw() {
      ctx.fillStyle = this.color;
      ctx.fillRect(this.x0*canvas.width, this.y0*canvas.height,
	           this.x1*canvas.width, this.y1*canvas.height);
   }
   overlap(x,y) {
      return ((x>=this.x0) && (x<=(this.x1+this.x0))) && y>=this.y0 && y<=this.y1+this.y0;
   }
   handleTouchCapture(x,y) {
      console.log("handleTouchCapture " + this.id);
   }
   handleTouchCapturedMove(x,y) {
      console.log("handleTouchCaptureMove " + this.id);
   }
   handleTouchCapturedEnd() {
      console.log("handleTouchCaptureEnd " + this.id);
   }
   handleTouch(x,y) {
      console.log("handleTouch " + this.id);
   }
   handleTouchMove(x,y) {
      console.log("handleTouchMove " + this.id);
   }
   handleTouchEnd() {
      console.log("handleTouchEnd " + this.id);
   }
   // may want to add more handlers for enter / leave without capture?

   get() {
      // returns string to send to server
      // if "", then send nothing
      return "";
   }
}

class Knob extends Element {
   constructor(id,x0,y0,x1,y1,type="direction",sensitivity=0.05) {
      super(id,x0,y0,x1,y1);
      this.color = "#AAAA00"
      this.type = type;
      this.drag = false;
      this.x0_ = 0;
      this.y0_ = 0;
      this.x1_ = 0;
      this.y1_ = 0;
      this.dmax = sensitivity;//directon distance - sensitivity
   }
   handleTouchCapture(x,y) {
      this.color = "#CCCC00"
      this.drag = true;
      this.x0_ = x;
      this.y0_ = y;
      this.x1_ = x;
      this.y1_ = y;
   }
   handleTouchCapturedMove(x,y) {
      this.color = rgb(255*x,255*y,0);
      this.x1_ = x;
      this.y1_ = y;
      switch(this.type) {
         case "direction":
            var dx = this.x1_ - this.x0_;
            var dy = this.y1_ - this.y0_;
            var d = Math.sqrt(dx*dx + dy*dy);
            if(d>this.dmax) {
               this.x0_ = this.x1_ - dx*this.dmax/d;
               this.y0_ = this.y1_ - dy*this.dmax/d;
            }
	    break;
	 case "slide":
            break;
         default:
	    console.log("Error: bad knob type " + this.type);
      }
   }
   handleTouchCapturedEnd() {
      this.color = "#AAAA00"
      this.drag = false;
   }
   get() {
      switch(this.type) {
         case "direction":
            var dx = String((this.x1_-this.x0_)/this.dmax).substring(0,7);
            var dy = String((this.y1_-this.y0_)/this.dmax).substring(0,7);
            return this.drag ? (dx+","+dy) : "f";
	 case "slide":
            var dx = String(this.x1_-this.x0_).substring(0,7);
            var dy = String(this.y1_-this.y0_).substring(0,7);
	    this.x0_=this.x1_;
	    this.y0_=this.y1_;
	    this.x0_=this.x1_;
            return this.drag ? (dx+","+dy) : "f";
         default:
	    console.log("Error: bad knob type " + this.type);
	    return "";
      }
   }
   draw() {
      super.draw()
      if(this.type == "direction" && this.drag) {
         ctx.fillStyle = "#000000";
         var xx = (this.x0_)*canvas.width;
         var yy = (this.y0_)*canvas.height;
         var rx = this.dmax * canvas.width;
         var ry = this.dmax * canvas.height;
         ctx.fillRect(xx-rx, yy-ry, 2*rx, 2*ry);
      }
   }
}

class Button extends Element {
   constructor(id,x0,y0,x1,y1) {
      super(id,x0,y0,x1,y1);
      this.color = "#00AAAA"
      this.drag = false;
      this.canCapture = true;
   }
   handleTouchCapture(x,y) {
      this.drag = true;
   }
   handleTouchCapturedMove(x,y) {
      this.color = rgb(0,255*x,255*y);
   }
   handleTouchCapturedEnd() {
      this.drag = false;
      this.color = "#00AAAA"
   }
   get() {
      return this.drag ? "t" : "f";
   }
}

function loadElements(s) {
   if(s!=elementsString) {
      elementsString = s;
      console.log("Load elements...");
      var els = s.split(";");
      newIds = {}
      for(var i=0; i<els.length; i++) {
         var eParts = els[i].split(":");
	 var eKind = eParts[0];
	 var eId = eParts[1];
	 var eP = eParts[2].split(",");
         
	 newIds[eId] = 1;

	 if(eId in elements) {
	 }else{
	    switch(eKind) {
	       case "kd": // knob direction
	          console.log(eId + " Knob direction")
	          elements[String(eId)] = new Knob(eId,eP[0],eP[1],eP[2],eP[3],"direction",eP[4]);
	          break;
	       case "ks": // knob slide
	          console.log(eId + " Knob slide")
	          elements[String(eId)] = new Knob(eId,eP[0],eP[1],eP[2],eP[3],"slide",eP[4]);
	          break;
	       case "b": // button
	          console.log(eId + " Button")
	          elements[String(eId)] = new Button(eId,eP[0],eP[1],eP[2],eP[3]);
	          break;

	       default:
	          console.log(eId + " error! " + els[i])
	    }
	 }
      }
      for(var i in elements) {
         if(!(i in newIds)) {
	    console.log("delete "+i)
            delete elements[i];
	 }
      }
   }
}

function drawElements() {
   for(var i in elements) {
      elements[i].draw();
   }
}

function findElement(x,y) {
   for(var i in elements) {
      if(elements[i].overlap(x,y)) {return i;}
   }
   return undefined;
}

function getElementsData() {
   var list = []
   for(var i in elements) {
      var s = elements[i].get();
      if(s!="") {
         list.push(i+"="+s)
      }
   }
   return list.join("&");
}



//loadElements("ks:0:0.1,0.1,0.8,0.2;kd:3:0.1,0.6,0.8,0.3");
//loadElements("ks:0:0.1,0.1,0.8,0.2;kd:4:0.1,0.4,0.8,0.2;b:5:0.1,0.7,0.3,0.2;b:6:0.6,0.7,0.3,0.2");

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

function rgb(r, g, b){
   return ["rgb(",r,",",g,",",b,")"].join("");
}





var ntouches = 20;
var tdata = {};

function initTData() {
   var i;
   for(i=-1; i<ntouches; i++) {
      tdata[i] = {
         drag: false,
	 x0: 0,
	 y0: 0,
	 x1: 0,
	 y1: 0,
	 color: rgb(i*30 % 256, i*50 % 256, i*70 % 256),
	 element: undefined, // if captured
      }
   }
}
initTData();

function tDataStart(id, x, y) {
   tdata[id].drag = true;
   tdata[id].x0 = (x - canvas.offsetLeft)/canvas.width;
   tdata[id].y0 = (y - canvas.offsetTop)/canvas.height;
   tdata[id].x1 = tdata[id].x0;
   tdata[id].y1 = tdata[id].y0;
   
   var e = findElement(tdata[id].x0,tdata[id].y0);
   console.log("touch: " + e);
   if(e!=undefined && e in elements) {
      var cc = elements[e].canCapture;
      var ic = elements[e].isCaptured;
      if(cc && ic==undefined) {
         // capture it myself
         tdata[id].element = e;
         elements[e].isCaptured = id;
         elements[e].handleTouchCapture(tdata[id].x0,tdata[id].y0);
      }
      if(!cc) {
         // inform it
         elements[e].handleTouch(tdata[id].x0,tdata[id].y0);
      }
   }
}
canvas.addEventListener("touchstart", function(event) {
   event.preventDefault();
   var touches = event.changedTouches;
   for(var i=0; i<touches.length; i++) {
      var id = touches[i].identifier;
      
      tDataStart(id,touches[i].pageX,touches[i].pageY);
   }
});
canvas.addEventListener("mousedown", function(event) {
   tDataStart(-1, event.pageX,event.pageY);
});


function tDataMove(id, x, y) {
   if(!tdata[id].drag) {return;}
   tdata[id].x1 = (x - canvas.offsetLeft)/canvas.width;
   tdata[id].y1 = (y - canvas.offsetTop)/canvas.height;
   
   if(tdata[id].element != undefined) {
      // we were captured
      var e = tdata[id].element;
      if(e in elements) {
         elements[e].handleTouchCapturedMove(tdata[id].x1,tdata[id].y1);
      }
   } else {
      // just inform who we are over:
      var e = findElement(tdata[id].x1,tdata[id].y1);
      if(e!=undefined && e in elements) {
         var cc = elements[e].canCapture;
         if(!cc) {
            elements[e].handleTouchMove(tdata[id].x1,tdata[id].y1)
         }
      }
   }
}
canvas.addEventListener("touchmove", function(event) {
   var touches = event.changedTouches;
   for(var i=0; i<touches.length; i++) {
      var id = touches[i].identifier;
      
      tDataMove(id, touches[i].pageX,touches[i].pageY);
   }
});
canvas.addEventListener("mousemove", function(event) {
   tDataMove(-1, event.pageX,event.pageY);
});


function tDataEnd(id) {
   tdata[id].drag = false;
   
   if(tdata[id].element != undefined) {
      // we were captured
      var e = tdata[id].element;
      if(e in elements) {
         elements[e].handleTouchCapturedEnd();
         elements[e].isCaptured = undefined;
         tdata[id].element = undefined;
      }
   } else {
      // just inform who we are over:
      var e = findElement(tdata[id].x1,tdata[id].y1);
      if(e!=undefined && e in elements) {
         var cc = elements[e].canCapture;
         if(!cc) {
            elements[e].handleTouchEnd(tdata[id].x1,tdata[id].y1)
         }
      }
   }
}
canvas.addEventListener("touchend", function(event) {
   var touches = event.changedTouches;
   for(var i=0; i<touches.length; i++) {
      var id = touches[i].identifier;
      
      tDataEnd(id);
   }
});
canvas.addEventListener("mouseup", function(event) {
   tDataEnd(-1);
});


var cnt = 0;
function run() {
   cnt++;
   
   var s = getElementsData()
   if(s!="") {s="&"+s}
   var url = "data?uid="+getCookie("userid")+s;
   fetch(url)
   .then(data => {return data.text()})
   .then(txt => {
      if(txt.startsWith("#")) {
	 if(txt=="#ok") {
            loadElements("");
	 } else {
            console.log("data error: "+txt);
	 }
      } else {
         loadElements(txt);
      }
   })
   .catch(error => console.log(error))

   clear();
   drawElements();
}

run()

var timer = setInterval(run, 50);

clear();



