<?php
/**
 * @file
 * Replacements for certain pages for the dual-language system.
 */

/**
 * Callback function to redirect to the English version of a page.
 *
 * @param object $node
 *   The node to redirect to
 */
function uottawa_lang_editor_redirect_english($node) {
  $list = language_list();
  $id = $node->nid;
  // if this is not the English node, we want the $tnid version
  if ($node->language != "en") {
    $id = $node->tnid;
  }
  // redirect cleanly
  header("Location: " . url("node/" . $id, array(
        "language" => $list["en"],
        "absolute" => TRUE
      )));
  drupal_exit("redirecting");
}

/**
 * Callback function to redirect to the French version of a page.
 *
 * @param object $node
 *   The node to redirect to.
 */
function uottawa_lang_editor_redirect_french($node) {
  $list = language_list();
  $id = $node->nid;
  // if this is not the French node, we need to look it up
  if ($node->language != "fr") {
    $trans = translation_node_get_translations($node->tnid);
    $id = $trans["fr"]->nid;
  }
  // redirect cleanly
  header("Location: " . url("node/" . $id, array(
        "language" => $list["fr"],
        "absolute" => TRUE
      )));
  drupal_exit("redirecting");
}

/**
 * Shows the difference between two nodes, in a dual-language context.
 *
 * @param array $nodes
 *   An array of nodes. Should be two English and two French nodes.
 *
 * @return string
 *   HTML content as returned by the theming function.
 */
function uottawa_lang_editor_view_diff($nodes) {
  $eng_nodes = array();
  $fra_nodes = array();
  foreach ($nodes as $node) {
    if ($node->language == "en") {
      $eng_nodes[] = $node;
    }
    else {
      $fra_nodes[] = $node;
    }
  }
  $eng_diff = uottawa_lang_editor_build_diff($eng_nodes);
  $fra_diff = uottawa_lang_editor_build_diff($fra_nodes);
  if (!empty($eng_diff) || !empty($fra_diff)) {
    return theme('side_by_side', array(
          'leftside' => $eng_diff,
          'rightside' => $fra_diff
        ));
  }
  else {
    drupal_set_message(t('Could not find any matching nodes to build a
      difference view from or the diff module is disabled'), 'error');
  }
}

/**
 * Constructs a diff between two nodes.
 *
 * @param array $nodes
 *   An array of nodes to diff. Must contain at least two nodes.
 *
 * @return boolean|string
 *   Either the HTML content of the diff or FALSE if a diff could not be
 *   generated.
 */
function uottawa_lang_editor_build_diff($nodes) {
  if (!module_exists('diff')) {
    return FALSE;
  }
  $nid = 0;
  $first = NULL;
  $vid1 = 0;
  $vid2 = 0;
  foreach ($nodes as $node) {
    if (empty($nid)) {
      $nid = $node->nid;
      $first = $node;
    }
    if ($node->nid == $nid) {
      if (empty($vid1)) {
        $vid1 = $node->vid;
      }
      else {
        if ($vid1 > $node->vid) {
          $vid2 = $vid1;
          $vid1 = $node->vid;
        }
        else {
          $vid2 = $node->vid;
        }
      }
    }
  }
  if (!empty($nid) && !empty($vid1) && !empty($vid2)) {
    module_load_include("inc", "diff", "diff.pages");
    return diff_diffs_show($first, $vid1, $vid2);
  }
  return FALSE;
}