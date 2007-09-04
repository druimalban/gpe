<?php
  if (empty($author)) {
    $author = "GPE Hackers";
    $authoremail = "gpe-list@linuxtogo.org";
  }
?>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" "http://www.w3.org/TR/REC-html40/loose.dtd">
<html>
<head>
<title>GPE: The GPE Palmtop Environment</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<link rel="shortcut icon" href="http://gpe.linuxtogo.org/images/favicon.png" type="image/png">
<link href="css/style.css" rel="stylesheet" type="text/css">
</head>
<body><script type="text/javascript">
if (document.images)
   {
     pic1on= new Image(100,28);
     pic1on.src="images/Home-o.jpg";  
     pic2on= new Image(100,28);
     pic2on.src="images/Projects-o.jpg";
     pic3on= new Image(100,27);
     pic3on.src="images/Documentation-o.jpg";  
     pic4on= new Image(100,28);
     pic4on.src="images/Download-o.jpg";
     pic5on= new Image(100,28);
     pic5on.src="images/Contact-o.jpg";
     pic6on= new Image(100,28);
     pic6on.src="images/Resources-o.jpg"


     pic1off= new Image(100,28);
     pic1off.src="images/Home.jpg";
     pic2off= new Image(100,28);
     pic2off.src="images/Projects.jpg";
     pic3off= new Image(100,27);
     pic3off.src="images/Documentation.jpg";
     pic4off= new Image(100,28);
     pic4off.src="images/Download.jpg";
     pic5off= new Image(100,28);
     pic5off.src="images/Contact.jpg";
     pic6off= new Image(100,28);
     pic6off.src="images/Resources.jpg";

   }

function lightup(imgName)
 {
   if (document.images)
    {
      imgOn=eval(imgName + "on.src");
      document[imgName].src= imgOn;
    }
 }

function turnoff(imgName)
 {
   if (document.images)
    {
      imgOff=eval(imgName + "off.src");
      document[imgName].src= imgOff;
    }
 }
</script>

<div class="header">
<a href="http://gpe.linuxtogo.org/">
<img class="logo" src="images/logo3s.png" alt="[The GPE Logo] > gpe.linuxtogo.org"></a>
<a href="http://www.linuxtogo.org/"><img class="logow" src="images/logo-w.png" alt="[The Linuxtogo.org] > www.linuxtogo.org"></a>
</div>

<div class="menu" align="left">
<ul class="menu">
<li><a href="http://gpe.linuxtogo.org/"onMouseover="lightup('pic1')" onMouseout="turnoff('pic1')"><img src="images/Home.jpg" alt="Home" name="pic1" width="100" height="28"></a></li>
<li><a href="http://gpe.linuxtogo.org/projects/"onMouseover="lightup('pic2')" onMouseout="turnoff('pic2')"><img src="images/Projects.jpg" alt="Projects" name="pic2" width="100" height="28"></a></li>
<li><a href="http://gpe.linuxtogo.org/documentation.phtml"onMouseover="lightup('pic3')" onMouseout="turnoff('pic3')"><img src="images/Documentation.jpg" alt="Documentation" name="pic3" width="100" height="27" id="pic3"></a></li>
<li><a href="http://gpe.linuxtogo.org/download.phtml"onMouseover="lightup('pic4')" onMouseout="turnoff('pic4')"><img src="images/Download.jpg" alt="Download" name="pic4" width="100" height="28" id="pic4"></a></li>
<li><a href="http://gpe.linuxtogo.org/contact.phtml"onMouseover="lightup('pic5')" onMouseout="turnoff('pic5')"><img src="images/Contact.jpg" alt="Contact" name="pic5" width="100" height="28" id="pic5"></a></li>
<li><a href="http://gpe.linuxtogo.org/linkres.phtml"onMouseover="lightup('pic6')" onMouseout="turnoff('pic6')"><img src="images/Resources.jpg" alt="Resources" name="pic6" width="100" height="28" id="pic6"></a></li>
</ul>
</div>
<hr>
<div class="content">

