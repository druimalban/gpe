<?php

function menu_entry($page, $title, $link,$thispage)
{
  if ($page == $thispage) {
    echo "<li><a href=\"#\" id=\"active\">$title</a></li>";
  } else {
    echo "<li><a href='$link'>$title</a></li>";
  }
}

?>

<div class="menu" align="center">
<ul class="menu">
<?php menu_entry("home", "Home", "/", $pagename);?>
 &middot;
<?php menu_entry("projects", "Projects", "/projects/", $pagename); ?>
 &middot;
<?php menu_entry("documentation", "Documentation", "/documentation.phtml", $pagename); ?>
 &middot;
<?php menu_entry("download", "Download", "/download.phtml", $pagename); ?>
 &middot;
<?php menu_entry("contact", "Contact", "/contact.phtml", $pagename); ?>
 &middot;
<li><A href="http://bugs.linuxtogo.org">Bugs</A></li>
 &middot;
<li><A href="http://projects.linuxtogo.org/projects/gpe/">Development</a>
 &middot;
<?php menu_entry("linkres", "Resources", "/linkres.phtml", $pagename); ?>
</ul>
</div>
