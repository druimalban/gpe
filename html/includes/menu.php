<?php

function menu_entry($page, $title, $link)
{
  if ($page == $pagename) {
    echo "<b>$title</b>";
  } else {
    echo "<a href='$link'>$title</a>";
  }
}

?>

<div class="menu">
<small>

<?php menu_entry("home", "Home", "/"); ?>
 |
<?php menu_entry("projects", "Projects", "/projects/"); ?>
 |
<?php menu_entry("documentation", "Documentation", "/documentation.shtml"); ?>
 |
<?php menu_entry("screenshots", "Screenshots", "/screenshots/"); ?>
 |
<?php menu_entry("download", "Download", "/download.shtml"); ?>
 |
<?php menu_entry("contact", "Mailing list", "/contact.shtml"); ?>
 |
<?php menu_entry("screenshots", "Screenshots", "/screenshots/"); ?>
 |
<A href="http://handhelds.org/bugzilla/">Bugs</A>
 |
<?php menu_entry("cvs", "CVS", "/cvs.shtml"); ?>
 |
<?php menu_entry("Links/Resources", "linkres", "/linkres.shtml"); ?>

</small>
</div>
