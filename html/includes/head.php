<?php
  if (empty($author)) {
    $author = "GPE Hackers";
    $authoremail = "gpe-list@linuxtogo.org";
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
<div id="website">
<div id="header"><table border="0" width="100%">
<tr>
<td width="60" height="64">
<a href="http://gpe.linuxtogo.org">
<img src="/images/gpe-logo2.png" alt="[The GPE Logo] > gpe.handhelds.org">
</td>
<td width="*" valign="middle" height="64" halign="middle">
<h1> GPE: <?php echo $addtitle; ?></h1>
</td>
<td halign="right">
<a href="http://www.linuxtogo.org">
<img src="/images/logo-w.png" alt="[The linuxtogo.org Logo]> linuxtogo.org">
</a>
</td>
</tr>
</table>
</div>
<?php include("http://gpe.linuxtogo.org/includes/menu.php"); ?>
<hr>
<div id="content">