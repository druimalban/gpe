<?php
  if (empty($author)) {
    $author = "GPE Hackers";
    $authoremail = "gpe@handhelds.org";
  }
?>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" "http://www.w3.org/TR/REC-html40/loose.dtd">
<HTML> 
<HEAD> 
<TITLE> GPE: <?php echo $addtitle; ?></TITLE> 
<META http-equiv="Content-Type" content="text/html; charset=UTF-8">
<LINK REL="stylesheet" HREF="/css/style.css" TYPE="text/css">
<LINK REL="shortcut icon" HREF="/images/favicon.png" TYPE="image/png">
</HEAD>
<BODY>
<table border=0 width="100%">
<tr>
<td width=70>
<a href="http://gpe.handhelds.org">
<img src="/images/gpe-logo2.png" alt="[The GPE Logo] > gpe.handhelds.org">
</td>
<td width="*" valign="middle">
<h1> GPE: <?php echo $addtitle; ?></h1>
</td>
<td halign="right">
<a href="http://handhelds.org">
<img src="http://www.handhelds.org/images/handheld_icon_white.jpg" alt="[The handhelds.org Logo] > handhelds.org">
</a>
</td>
</tr>
</table>
<?php include("/home/gpe/website/html/includes/menu.php"); ?>
<hr>
