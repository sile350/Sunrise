  window.onerror = function (msg, url, line) {
               alert("Message : " + msg );
               alert("url : " + url );
               alert("Line number : " + line );
            }
function clearP($id)
{
$id.value="";	
}
function elChange($elem)
{
if($elem.name=="p1" && $elem.checked==true)
{
 
document.getElementById("idb1").innerHTML=$elem.value;	
}
if($elem.name=="p2" && $elem.checked==true)
{
 
document.getElementById("idb2").innerHTML=$elem.value;	
}
if($elem.name=="p3" && $elem.checked==true)
{
 
document.getElementById("idb3").innerHTML=$elem.value;	
}
if($elem.name=="p4" && $elem.checked==true)
{
 
document.getElementById("idb4").innerHTML=$elem.value;	
}
if($elem.name=="p5" && $elem.checked==true)
{
 
document.getElementById("idb5").innerHTML=$elem.value;	
}
if($elem.name=="p6" && $elem.checked==true)
{
 
document.getElementById("idb6").innerHTML=$elem.value;	
}
if($elem.name=="p21" && $elem.checked==true)
{
 
document.getElementById("idb21").innerHTML=$elem.value;	
}
if($elem.name=="p22" && $elem.checked==true)
{
 
document.getElementById("idb22").innerHTML=$elem.value;	
}
}
function calculateTest()
{
 if(document.getElementById("idp1").value=="" || document.getElementById("idp2").value==""|| document.getElementById("idp2").value=="  ?" || document.getElementById("idp1").value=="  ?")

{ 
alert("Ошибка! Не все данные внесены.");
return;
}
 else
var rez=document.getElementById("idp1").value*document.getElementById("idp2").value/20;
 
document.getElementById("idikr").value=rez;	
 
}
function checkEmpty($elem,$word)
{
 
if($elem.innerHTML=="")
{
$elem.style.color="#999";	
$elem.innerHTML=$word;
}
}
//щелкнули по назначению
function beginNaz($id)
{
 
 if($id.style.color!="#000000" && $id.style.color!="black" )
 {
$id.innerHTML="";
$id.style.color="#000000";	
 }
}

function hideBox()
{
document.getElementById("idbox").style.display="none";	
}
function showBox($text)
{
 
   var e = window.event;	
  var posX = e.clientX;
  var supportPageOffset = window.pageXOffset !== undefined;
var isCSS1Compat = ((document.compatMode || "") === "CSS1Compat");

 
var y = supportPageOffset ? window.pageYOffset : isCSS1Compat ? document.documentElement.scrollTop : document.body.scrollTop;
    var posY = e.clientY+y;
posY=posY-50;
posX=posX+20;
document.getElementById("idbox").style.display="block";
document.getElementById("idbox").style.left=posX+"px";
document.getElementById("idbox").style.top=posY+"px";
document.getElementById("idbox").innerHTML=$text;
}
function showBoxLong($text)
{
 
   var e = window.event;	
  var posX = e.clientX;
  var supportPageOffset = window.pageXOffset !== undefined;
var isCSS1Compat = ((document.compatMode || "") === "CSS1Compat");

 
var y = supportPageOffset ? window.pageYOffset : isCSS1Compat ? document.documentElement.scrollTop : document.body.scrollTop;
    var posY = e.clientY+y;
posY=posY-250;
posX=posX+20;
document.getElementById("idbox").style.display="block";
document.getElementById("idbox").style.left=posX+"px";
document.getElementById("idbox").style.top=posY+"px";
document.getElementById("idbox").innerHTML=$text;
}
function showBoxLeft($text,$raz)
{
 
   var e = window.event;	
  var posX = e.clientX;
  var supportPageOffset = window.pageXOffset !== undefined;
var isCSS1Compat = ((document.compatMode || "") === "CSS1Compat");

 
var y = supportPageOffset ? window.pageYOffset : isCSS1Compat ? document.documentElement.scrollTop : document.body.scrollTop;
    var posY = e.clientY+y;
posY=posY-50;
posX=posX-$raz;
document.getElementById("idbox").style.display="block";
document.getElementById("idbox").style.left=posX+"px";
document.getElementById("idbox").style.top=posY+"px";
document.getElementById("idbox").innerHTML=$text;
}
function eswitch($id)
{ 
$tid="s"+$id;
if(document.getElementById($tid).style.display=="none")
document.getElementById($tid).style.display="block";	
else
document.getElementById($tid).style.display="none";	
}