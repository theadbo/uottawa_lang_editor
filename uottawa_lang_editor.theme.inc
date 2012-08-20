<?php
/**
 * @file
 * Provides basic theme implementations for the dual-language editor.
 */

/**
 * Themes a list of revisions form to have diff checkboxes.
 *
 * @param array $vars
 *   Variables array. Must have an entry called 'form'.
 *
 * @return string
 *   HTML output.
 */
function theme_uottawa_lang_editor_diff($vars) {
  $form = $vars['form'];
  $output = '';

  // Overview table:
  $header = array(
    'eng' => t('English Revision'),
    'fra' => t('French Revision'),
    array('data' => drupal_render($form['submit']), 'colspan' => 2),
    array('data' => t('Revert')),
    array('data' => t('Delete')),
  );
  $diff_keys = array_keys($form['diff']['old']['#options']);
  $rows = array();
  if (isset($form['info']) && is_array($form['info'])) {
    foreach (element_children($form['info']) as $key) {
      $row = array();
      // Note: even if the commands for revert and delete are not permitted,
      // the array is not empty since we set a dummy in this case.
      $row['eng'] = drupal_render($form['info'][$key]);
      $row['fra'] = drupal_render($form['info_fra'][$key]);
      $row[] = drupal_render($form['diff']['old'][$diff_keys[$key]]);
      $row[] = drupal_render($form['diff']['new'][$diff_keys[$key]]);
      $row[] = drupal_render($form['operations'][$key][0]);
      $row[] = drupal_render($form['operations'][$key][1]);
      $rows[] = $row;
    }
  }
  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  $output .= drupal_render_children($form);

  return $output;
}

/**
  * Adds JS and CSS code needed for a dual-language view
  */
function uottawa_lang_editor_include_side_by_side_css() {
  drupal_add_js(drupal_get_path('module', 'uottawa_lang_editor') .
      '/uottawa_lang_editor_js.js');
  drupal_add_css(drupal_get_path('module', 'uottawa_lang_editor') .
      '/uottawa_lang_editor_css.css');
}

/**
 * Themes a side-by-side view for two blocks of content
 * @param array $variables
 *   List of variables. Must specify at least one of leftside or rightside.
 *
 * @return string
 *   HTML content.
 */
function theme_side_by_side($variables) {
  if (!empty($variables['leftside']) && !empty($variables['rightside'])) {
    uottawa_lang_editor_include_side_by_side_css();
    $html = '<div class="uottawa_lang_editor_english uottawa_lang_editor_preview"
      id="preview-english-block"><p class="uottawa_lang_editor_actions">
<span id="uottawa_lang_editor_collapse_english_preview"
class="uottawa_lang_editor_collapse_preview">';
    $html .= t($variables['collapse_text']);
    $html .= '</span>';
    $html .= '<span id="uottawa_lang_editor_expand_english_preview" class="
uottawa_lang_editor_expand_preview">';
    $html .= t($variables['expand_text']);
    $html .= '</span>';
    $html .= '</p>';
    $html .= $variables['leftside'];
    $html .= '</div>';
    $html .= '<div class="uottawa_lang_editor_french
uottawa_lang_editor_preview" id="preview-french-block">';
    $html .= '<p class="uottawa_lang_editor_actions">';
    $html .= '<span id="uottawa_lang_editor_collapse_french_preview"
class="uottawa_lang_editor_collapse_preview">';
    $html .= t($variables['collapse_text']);
    $html .= '</span>';
    $html .= '<span id="uottawa_lang_editor_expand_french_preview"
class="uottawa_lang_editor_expand_preview">';
    $html .= t($variables['expand_text']);
    $html .= '</span>';
    $html .= '</p>';
    $html .= $variables['rightside'];
    $html .= '</div><br style="clear: both;" /><hr />';
    return $html;
  }
  elseif (!empty($variables['leftside'])) {
    return $variables['leftside'];
  }
  elseif (!empty($variables['rightside'])) {
    return $variables['rightside'];
  }
  else {
    drupal_set_message(t('Warning, neither left-hand nor right-hand theme
      elements set in theme_side_by_side'));
    return '';
  }
}
