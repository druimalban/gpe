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
<LINK REL="shortcut icon" HREF="/images/favicon.ico" TYPE="image/x-icon">
</HEAD>
<BODY>
<table border=0>
<tr>
<td width=70>
<img src="/images/gpe-logo.png" alt="[The GPE Logo]">
</td>
<td width="*" valign="middle">
<h1> GPE: <?php echo $addtitle; ?></h1>
</td> 
</tr>
</table>
<?php include("menu.php"); ?>
<hr>
